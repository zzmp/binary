CXX=g++
CPP_FLAGS=-std=c++11
CPLEX_FLAGS=-DIL_STD
CPLEX_DIR=/opt/ibm/ILOG/CPLEX_Studio_Community127
CPLEX_INCL=-I$(CPLEX_DIR)/concert/include -I$(CPLEX_DIR)/cplex/include
CPLEX_LIB_DIRS=-L$(CPLEX_DIR)/concert/lib/x86-64_linux/static_pic -L$(CPLEX_DIR)/cplex/lib/x86-64_linux/static_pic
CPLEX_LIBS=$(CPLEX_LIB_DIRS) -lilocplex -lconcert -lcplex -lm -lpthread

SRCS=solver.cpp

all: solver

solver: $(SRCS)
	$(CXX) -o solver $(SRCS) $(CPP_FLAGS) $(CPLEX_FLAGS) $(CPLEX_INCL) $(CPLEX_LIBS)

clean:
	rm -f solver
