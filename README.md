boolean
---

A binary to find the goal value of boolean functions.

`./solver`, when compiled, will iterate through all combinations
of an n-length boolean function and use cplex to find the minimum goal value.

### Compiling

You will need CPLEX, from IBM.
The Makefile assumes that this is installed in `/opt/ibm/ILOG/CPLEX_Studio_Community127`.
Pass in the correct paths to the Makefile if this is true.

Compile with `make`.

### Running

- `./solver n` will solve functions with n.
- `./solver min max` will solve functions from n=min..max.
- `./solver --export-models` will dump `.lp`-formatted models into `./models/` for inspection (aliased `-e`).
- `./solver --singleton VECTOR` will solve for VECTOR (ex.: 0001000) (aliased `-s`).
- `./solver --verbose` will display the solution matrix for given functions (aliased `-v`).
