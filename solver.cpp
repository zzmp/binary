#include <functional>
#include <iostream>

#include "ilconcert/iloenv.h"
#include "ilconcert/ilomodel.h"
#include "ilcplex/ilocplexi.h"

class Env {
public:
    ~Env() { _env.end(); }
    operator IloEnv() const { return _env; }
private:
    IloEnv _env;
};

void permute(int len, std::function<void(bool[], int len)> lambda, int bit = 0);
void generate_model(bool bits[], int len);

int main(int argc, char** argv) {
    int max_len;
    if (argc == 1) {
        max_len = 8;
    } else if (argc == 2) {
        max_len = std::atoi(argv[1]);
        if (max_len < 2 || max_len > 16) exit(1);
    } else {
        exit(1);
    }

    for (int len = 2; len <= max_len; ++len)
        permute(len, generate_model);
}

void generate_model(bool bits[], int len) {
    int max = len - 1;

    std::string name;
    { // get bit-representation as string
        std::stringstream rep;
        for (int i = 0; i < len; ++i) rep << (bits[i] ? "1" : "0");
        name = rep.str();
    }

    // create the environment
    Env env;

    IloModel model(env);
    IloArray<IloIntVarArray> x(env);

    { // create variables
        for (int i = 0; i < len; ++i) {
            IloIntVarArray tmp(env);
            for (int j = 0; j < len; ++j) tmp.add(IloIntVar(env));
            x.add(tmp);
        }
    }

    { // create general constraints
        x[0][0].setUB(0);
        IloRangeArray constraints(env);
        for (int i = 0; i < len; ++i) {
            for (int j = 0; j < len; ++j) {
                // x[i][j] <= x[i+1][j]
                if (i < max) constraints.add(x[i][j] - x[i+1][j] <= 0);

                // x[i][j] <= x[i][j+1]
                if (j < max) constraints.add(x[i][j] - x[i][j+1] <= 0);

                if (i < max && j < max) {
                    // x[i][j+1] - x[i][j] >= x[i+1][j+1] - x[i+1][j]
                    constraints.add(x[i][j+1] - x[i][j] - x[i+1][j+1] + x[i+1][j] >= 0);

                    // x[i+1][j] - x[i][j] >= x[i+1][j+1] - x[i][j+1]
                    constraints.add(x[i+1][j] - x[i][j] - x[i+1][j+1] + x[i][j+1] >= 0);
                }

                // x[i+1][j] - x[i][j] >= x[i+2][j] - x[i+1][j]
                if (i < max - 1) constraints.add(x[i+1][j] - x[i][j] - x[i+2][j] + x[i+1][j] >= 0);

                // x[i][j+1] - x[i][j] >= x[i][j+2] - x[i][j+1]
                if (j < max - 1) constraints.add(x[i][j+1] - x[i][j] - x[i][j+2] + x[i][j+1] >= 0);
            }
        }
        model.add(constraints);
    }

    { // create permutation-specific constraints
        IloConstraintArray termini(env);

        auto for_termini = [&bits, &len, &max](std::function<void(int,int)> lambda) {
            // TODO: review
            int low_bit = 0;
            bool pattern = bits[low_bit];
            int trues, falses;

            for (int i = 1; i < len; ++i) {
                bool bit = bits[i];
                if (bit == pattern) continue;

                trues = low_bit;
                falses = max - (i - 1);

                low_bit = i;
                pattern = bit;

                lambda(falses, trues);
            }

            trues = low_bit;
            falses = 0;
            lambda(falses, trues);
        };

        for_termini([&termini, &x, &max](int i, int j) {
            termini.add(x[i][j] == x[max][max]);
            if (i > 0) termini.add(x[i][j] - x[i-1][j] > 0);
            if (j > 0) termini.add(x[i][j] - x[i][j-1] > 0);
        });

        model.add(termini);
    }

    { // create the objective function
        model.add(IloMinimize(env, x[len - 1][len - 1]));
    }

    {
        IloCplex cplex(model);
        cplex.setOut(((IloEnv)env).getNullStream());

        // TODO: add a toggle to save the model
        /*
        std::string filepath("models/");
        filepath += len - 48;
        filepath += ".";
        filepath += name;
        filepath += ".lp";
        cplex.exportModel(filepath.c_str());
        */

        // compute
        bool success = cplex.solve();

        // report
        std::cout << len << '\t' << name << '\t' << cplex.getStatus();
        if (success) std::cout << '\t' << cplex.getObjValue();
        std::cout << std::endl;
    }
}

void permute(int len, std::function<void(bool[], int)> lambda, int bit) {
    assert(len <= 16);
    static bool bits[16];
    if (bit < len) {
        bits[bit] = false;    
        permute(len, lambda, bit + 1);
        bits[bit] = true;
        permute(len, lambda, bit + 1);
    } else {
        // omit trivial solutions
        // they conflict with the constraint that x[0][0] == 0
        bool valid = false;
        bool bit = bits[0];
        for (int i = 0; i < len; ++i) {
            if (bits[i] != bit) {
                valid = true;
                break;
            }
        }

        if (valid) lambda(bits, len);
    }
}
