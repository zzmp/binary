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

bool export_models = false;
bool singleton = false;
bool verbose = false;

void permute(bool value[], int len, int bit = 0);
void solve(bool value[], int len);

int main(int argc, char** argv) {
    int arg_offset = 0;

    // parse options
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            if (std::strcmp(argv[i], "-s") == 0 || std::strcmp(argv[i], "--singleton") == 0) {
                singleton = true;
            } else if (std::strcmp(argv[i], "-e") == 0 || std::strcmp(argv[i], "--export-models") == 0) {
                export_models = true;
            } else if (std::strcmp(argv[i], "-v") == 0 || std::strcmp(argv[i], "--verbose") == 0) {
                verbose = true;
            }
        }
        if (verbose)
            arg_offset++;
        if (export_models) {
            arg_offset++;
            // create a models/ dir, if necessary
            mkdir("models", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        }
        if (singleton) {
            arg_offset++;
        }
    }

    if (argc <= (arg_offset + 1)) {
        std::cerr << "missing inputs" << std::endl;
        exit(1);
    }

    // solve
    if (singleton) {
        int len = std::strlen(argv[1 + arg_offset]);
        bool* value = new bool[len];
        for (int i = 0; i < len; ++i) {
            switch(argv[1 + arg_offset][i]) {
                case '1': value[i] = true; break;
                case '0': value[i] = false; break;
                default:
                    std::cerr << "bad singleton" << std::endl;
                    exit(1);
            }
        }
        solve(value, len);
        delete[] value;
    } else {
        int min_len, max_len;
        if (argc == (2 + arg_offset)) {
            int len = std::atoi(argv[1 + arg_offset]) + 1;
            min_len = max_len = len;
        } else if (argc = (3 + arg_offset)) {
            min_len = std::atoi(argv[1 + arg_offset]) + 1;
            max_len = std::atoi(argv[2 + arg_offset]) + 1;
        }
        bool* value = new bool[max_len];
        for (int i = 0; i < max_len; ++i) value[i] = false;
        for (int len = min_len ; len <= max_len; ++len) {
            permute(value, len);
        }
    }
}

// recursively permute through all solutions
void permute(bool value[], int len, int bit) {
    if (bit < len) {
        value[bit] = false;
        permute(value, len, bit + 1);
        value[bit] = true;
        permute(value, len, bit + 1);
    } else {
        bool valid = false;
        bool bit = value[0];
        for (int i = 0; i < len; ++i) {
            // only solve non-trivial solutions
            // trivial solutions conflict with the constraint that x[0][0] == 0
            if (value[i] != bit) {
                valid = true;
                solve(value, len);
                break;
            }
        }
    }
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

        // export, if necessary
        if (export_models) {
            std::stringstream filepath;
            filepath << "models/" << len << "." << name.str() << ".lp";
            cplex.exportModel(filepath.str().c_str());
        }

        // compute
        bool success = false;
        try {
            success = cplex.solve();
        } catch (IloCplex::Exception& e) {
            std::cerr << e << std::endl;
            exit(1);
        }

        // report
        std::cout << (len - 1) << '\t' << name.str() << '\t' << cplex.getStatus();
        if (success) std::cout << '\t' << cplex.getObjValue();
        std::cout << std::endl;

        if (verbose) {
            IloNumArray soln(env);
            for (int i = len-1; i >= 0; --i) {
                cplex.getValues(soln, x[i]);
                std::cout << '\t';
                for (int j = 0; j < len; ++j)
                    printf("%-3d", (int)soln[j]);
                std::cout << std::endl;
            }
        }
    }
}
