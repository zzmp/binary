boolean
---

A binary to find the goal value of boolean functions.

`./solver`, when compiled, will iterate through all combinations
of an n-length boolean function and use cplex to find the minimum goal value.

### Compiling

You will need CPLEX Community Edition, from IBM.
The Makefile assumes that this is installed in `/opt/ibm/ILOG/CPLEX_Studio_Community127`.

Compile with `make`.

### Running

By default, `./solver` will solve all boolean functions from n=1..7.

- `./solver m` will solve functions from n=1..m (m < 16).
- `./solver m1 m2` will solve functions from n=m1..m2 (m1 >= 1, m2 < 16).
- `./solver --export-models` will dump `.lp`-formatted models into `./models/` for inspection.
- `./solver -e` is an alias for `./solver --export-models`.
- `./solver --verbose XXXX` will solve for XXXX, display the solution, and dump an `.lp` model.
- `./solver -v XXXX` is an alias for `./solver --verbose XXXX`.
