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

void solve(bool value[], int len);
void permute_value(int len, std::function<void(bool[], int)> lambda, int bit = 0);

bool export_models = false;
bool verbose = false;

int main(int argc, char** argv) {
    int arg_offset = 0;
    if (argc > 1) {
        verbose = std::strcmp(argv[1], "-v") == 0 ||
            std::strcmp(argv[1], "--verbose") == 0;
        export_models = std::strcmp(argv[1], "-e") == 0 ||
            std::strcmp(argv[1], "--export-models") == 0;
        bool singleton = std::strcmp(argv[1], "-s") == 0 ||
            std::strcmp(argv[1], "--singleton") == 0 ||
            verbose;

        if (export_models) {
            mkdir("models", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            arg_offset = 1;
        }

        if (singleton) {
            int len = std::strlen(argv[2]);
            bool* value = new bool[len];
            for (int i = 0; i < len; ++i) {
                switch(argv[2][i]) {
                    case '1': value[i] = true; break;
                    case '2': value[i] = false; break;
                    default: exit(1);
                }
            }
            solve(value, len);
            delete[] value;

            exit(0);
        }
    }

    if (argc == (2 + arg_offset)) {
        int len = std::atoi(argv[1 + arg_offset]) + 1;
        permute_value(len, solve);
    } else if (argc == (3 + arg_offset)) {
        int min_len = std::atoi(argv[1 + arg_offset]) + 1;
        int max_len = std::atoi(argv[2 + arg_offset]) + 1;
        for (int len = min_len; len <= max_len; ++len)
            permute_value(len, solve);
    }

    exit(1);
}

void solve(bool value[], int len) {
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
                name << "_" << i << "_" << j << "_";
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
        IloConstraintArray constraints(env);

        // create a certificate vector
        std::vector<int> certificates(len);
        // the diagonal always has a certificate
        for (int i = 0; i < len; ++i)
            certificates[i] = max - i;

        // find the minimal certificates from contiguous values
        int low_value= 0;
        bool last_pattern = value[low_value];
        for (int i = 0; i < len; ++i) {
            bool pattern = value[i];
            if (pattern == last_pattern) continue;

            certificates[max - (i - 1)] = low_value;

            low_value= i;
            last_pattern = pattern;
        }
        certificates[0] = low_value;

        // the certificate vector should be monotonically decreasing
        for (int i = 1; i < len; ++i)
            certificates[i] = std::min(certificates[i-1], certificates[i]);

        for (int i = 0; i < len; ++i) {
            int j = certificates[i];
            // add a constraint on the maximal values (certificates)
            constraints.add(x[i][j] == x[max][max]);
            // add a constraint to values bordering certificates
            if (j > 0) constraints.add(x[max][max] - x[i][j-1] > 0);
        }

        model.add(constraints);
    }

    { // create the objective function
        model.add(IloMinimize(env, x[len - 1][len - 1]));
    }

    {
        std::stringstream name;
        for (int i = 0; i < len; ++i)
            name << (value[i] ? "1" : "0");

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

void permute_value(int len, std::function<void(bool[], int)> lambda, int bit) {
    assert(len <= 16);
    static bool value[16];
    if (bit < len) {
        value[bit] = false;
        permute_value(len, lambda, bit + 1);
        value[bit] = true;
        permute_value(len, lambda, bit + 1);
    } else {
        // omit trivial solutions
        // they conflict with the constraint that x[0][0] == 0
        bool valid = false;
        bool bit = value[0];
        for (int i = 0; i < len; ++i) {
            if (value[i] != bit) {
                valid = true;
                break;
            }
        }

        if (valid) lambda(value, len);
    }
}
