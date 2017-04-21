#include <functional>
#include <iostream>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>

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

void solve(bool bits[], int len);
void permute_bits(int len, std::function<void(bool[], int len)> lambda, int bit = 0);

bool export_models = false;
bool verbose = false;

int main(int argc, char** argv) {
    int arg_offset = 0;
    if (argc > 1) {
        bool should_export_models = std::strcmp(argv[1], "-e") == 0 ||
            std::strcmp(argv[1], "--export-models") == 0;
        bool should_be_verbose = std::strcmp(argv[1], "-v") == 0 ||
            std::strcmp(argv[1], "--verbose") == 0;

        if (should_export_models || should_be_verbose) {
            export_models = true;

            mkdir("models", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            arg_offset = 1;
        }

        if (should_be_verbose) {
            verbose = true;

            int len = std::strlen(argv[2]);
            bool* bits = new bool[len];
            for (int i = 0; i < len; ++i) {
                if (argv[2][i] == '0') {
                    bits[i] = false;
                } else if (argv[2][i] == '1') {
                    bits[i] = true;
                } else {
                    exit(1);
                }
            }
            solve(bits, len);
            delete[] bits;

            exit(0);
        }
    }

    if (argc == (2 + arg_offset)) {
        int len = std::atoi(argv[1 + arg_offset]) + 1;
        permute_bits(len, solve);
    } else if (argc == (3 + arg_offset)) {
        int min_len = std::atoi(argv[1 + arg_offset]) + 1;
        int max_len = std::atoi(argv[2 + arg_offset]) + 1;
        for (int len = min_len; len <= max_len; ++len)
            permute_bits(len, solve);
    }

    exit(1);
}

void solve(bool bits[], int len) {
    int max = len - 1;

    // create the environment
    Env env;

    IloModel model(env);
    IloArray<IloIntVarArray> x(env);

    { // create variables
        for (int i = 0; i < len; ++i) {
            IloIntVarArray tmp(env);
            for (int j = 0; j < len; ++j) {
                IloIntVar y(env);
                std::stringstream name;
                name << "x_" << i << "_" << j;
                y.setName(name.str().c_str());
                tmp.add(y);
            }
            x.add(tmp);
        }
    }

    { // create general constraints
        x[0][0].setUB(0);
        IloRangeArray constraints(env);
        for (int i = 0; i < len; ++i) {
            for (int j = 0; j < len; ++j) {
                // x[i][j] <= x[i+1][j]
                if (i < max)
                    constraints.add(x[i][j] - x[i+1][j] <= 0);

                // x[i][j] <= x[i][j+1]
                if (j < max)
                    constraints.add(x[i][j] - x[i][j+1] <= 0);

                // x[i+1][j] - x[i][j] >= x[i+2][j] - x[i+1][j]
                if (i < max - 1)
                    constraints.add(x[i+1][j] - x[i][j] - x[i+2][j] + x[i+1][j] >= 0);

                // x[i][j+1] - x[i][j] >= x[i][j+2] - x[i][j+1]
                if (j < max - 1)
                    constraints.add(x[i][j+1] - x[i][j] - x[i][j+2] + x[i][j+1] >= 0);

                // x[i][j+1] - x[i][j] >= x[i+1][j+1] - x[i+1][j]
                // x[i+1][j] - x[i][j] >= x[i+1][j+1] - x[i][j+1]
                if (i < max && j < max) {
                    constraints.add(x[i][j+1] - x[i][j] - x[i+1][j+1] + x[i+1][j] >= 0);
                    constraints.add(x[i+1][j] - x[i][j] - x[i+1][j+1] + x[i][j+1] >= 0);
                }
            }
        }
        model.add(constraints);
    }

    { // create permutation-specific constraints
        IloConstraintArray termini(env);

        auto for_termini = [&bits, &len, &max](std::function<void(int falses,int trues)> lambda) {
            int low_bit = 0;
            bool last_pattern = bits[low_bit];

            // find all contiguous solutions
            for (int i = 0; i < len; ++i) {
                bool pattern = bits[i];
                if (pattern == last_pattern) continue;

                lambda(max - (i - 1), low_bit);

                low_bit = i;
                last_pattern = pattern;
            }
            lambda(0, low_bit);
        };

        std::vector<int> minima_i(len);
        std::vector<int> minima_j(len);
        // the diagonal is always the maximal value
        for (int i = 0; i < len; ++i) {
            minima_i[i] = max - i;
            minima_j[i] = max - i;
        }

        for_termini([&minima_i, &minima_j, &x, &max](int i, int j) {
            // terminus should be less than the row's minimum
            minima_i[i] = j;
            minima_j[j] = i;
        });

        for (int i = 0; i < len; ++i) {
            int j = minima_i[i];

            // minimum should equal maximal value
            termini.add(x[i][j] == x[max][max]);

            // minimum should be greater than its antecedents
            if (i > 0 && minima_j[j] > i - 1) termini.add(x[max][max] - x[i-1][j] > 0);
            if (j > 0) termini.add(x[max][max] - x[i][j-1] > 0);
        }

        model.add(termini);
    }

    { // create the objective function
        model.add(IloMinimize(env, x[len - 1][len - 1]));
    }

    {
        std::stringstream name;
        for (int i = 0; i < len; ++i)
            name << (bits[i] ? "1" : "0");

        IloCplex cplex(model);
        // silence cplex
        cplex.setOut(((IloEnv)env).getNullStream());
        cplex.setWarning(((IloEnv)env).getNullStream());

        // export
        if (export_models) {
            std::stringstream filepath;
            filepath << "models/" << len << "." << name.str() << ".lp";
            cplex.exportModel(filepath.str().c_str());
        }

        // compute
        bool success = cplex.solve();

        // report
        std::cout << (len - 1) << '\t' << name.str() << '\t' << cplex.getStatus();
        if (success) std::cout << '\t' << cplex.getObjValue();
        std::cout << std::endl;

        if (verbose) {
            IloNumArray soln(env);
            for (int i = len-1; i >= 0; --i) {
                cplex.getValues(soln, x[i]);
                for (int j = 0; j < len; ++j)
                    std::cout << soln[j] << '\t';
                std::cout << std::endl;
            }
        }
    }
}

void permute_bits(int len, std::function<void(bool[], int)> lambda, int bit) {
    assert(len <= 16);
    static bool bits[16];
    if (bit < len) {
        bits[bit] = false;    
        permute_bits(len, lambda, bit + 1);
        bits[bit] = true;
        permute_bits(len, lambda, bit + 1);
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
