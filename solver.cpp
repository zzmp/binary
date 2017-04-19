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

int main() {
    // create the environment
    Env env;

    // create the problem
    IloModel model(env);
    const int ARRAY_SIZE = 6;
    const int ARRAY_MAX = ARRAY_SIZE - 1;
    IloArray<IloIntVarArray> x(env);

    { // create variables
        for (int i = 0; i < ARRAY_SIZE; ++i) {
            IloIntVarArray tmp(env);
            for (int j = 0; j < ARRAY_SIZE; ++j) tmp.add(IloIntVar(env));
            x.add(tmp);
        }
    }

    { // create constraints
        x[0][0].setUB(0);
        IloConstraintArray termini(env);
        termini.add(x[5][0] == x[5][5]);
        termini.add(x[3][1] == x[5][5]);
        termini.add(x[2][3] == x[5][5]);
        termini.add(x[4][0] == x[5][5]);

        termini.add(x[5][0] - x[4][0] > 0);
        termini.add(x[3][1] - x[2][1] > 0);
        termini.add(x[2][3] - x[1][3] > 0);
        termini.add(x[4][0] - x[3][0] > 0);
        model.add(termini);

        IloRangeArray constraints(env);
        for (int i = 0; i < ARRAY_SIZE; ++i) {
            for (int j = 0; j < ARRAY_SIZE; ++j) {
                // x[i][j] <= x[i+1][j]
                if (i < ARRAY_MAX) constraints.add(x[i][j] - x[i+1][j] <= 0);

                // x[i][j] <= x[i][j+1]
                if (j < ARRAY_MAX) constraints.add(x[i][j] - x[i][j+1] <= 0);

                if (i < ARRAY_MAX && j < ARRAY_MAX) {
                    // x[i][j+1] - x[i][j] >= x[i+1][j+1] - x[i+1][j]
                    constraints.add(x[i][j+1] - x[i][j] - x[i+1][j+1] + x[i+1][j] >= 0);

                    // x[i+1][j] - x[i][j] >= x[i+1][j+1] - x[i][j+1]
                    constraints.add(x[i+1][j] - x[i][j] - x[i+1][j+1] + x[i][j+1] >= 0);
                }

                // x[i+1][j] - x[i][j] >= x[i+2][j] - x[i+1][j]
                if (i < ARRAY_MAX - 1) constraints.add(x[i+1][j] - x[i][j] - x[i+2][j] + x[i+1][j] >= 0);

                // x[i][j+1] - x[i][j] >= x[i][j+2] - x[i][j+1]
                if (j < ARRAY_MAX - 1) constraints.add(x[i][j+1] - x[i][j] - x[i][j+2] + x[i][j+1] >= 0);
            }
        }
        model.add(constraints);
    }

    // create the objective function
    model.add(IloMinimize(env, x[ARRAY_MAX][ARRAY_MAX]));

    IloCplex cplex(model);

    // save
    cplex.exportModel("model.lp");

    // solve
    bool success = cplex.solve();
    std::cout << cplex.getStatus() << std::endl;
    if (success) {
        std::cout << cplex.getObjValue() << std::endl;

        IloNumArray soln(env);
        for (int i = 0; i < ARRAY_SIZE; ++i) {
            cplex.getValues(soln, x[i]);
            for (int j = 0; j < ARRAY_SIZE; ++j) std::cout << soln[j] << ' ';
            std::cout << std::endl;
        }

    }
}
