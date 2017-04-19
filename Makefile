CXX=g++
CPP_FLAGS=-std=c++11
CPLEX_FLAGS=-DIL_STD
CPLEX_INCL=-I/opt/ibm/ILOG/CPLEX_Studio_Community127/concert/include -I/opt/ibm/ILOG/CPLEX_Studio_Community127/cplex/include
CPLEX_LIB_DIRS=-L/opt/ibm/ILOG/CPLEX_Studio_Community127/concert/lib/x86-64_linux/static_pic -L/opt/ibm/ILOG/CPLEX_Studio_Community127/cplex/lib/x86-64_linux/static_pic
CPLEX_LIBS=$(CPLEX_LIB_DIRS) -lilocplex -lconcert -lcplex -lm -lpthread

SRCS=solver.cpp

all: solver

solver: $(SRCS)
	$(CXX) -o solver $(SRCS) $(CPP_FLAGS) $(CPLEX_FLAGS) $(CPLEX_INCL) $(CPLEX_LIBS)
