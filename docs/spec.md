
# XLS ideas

The basic structure is one of frontends and backends. Frontends handle user input and output, and create Unified Lifesrc Problem (ULP) files. Backends read ULP files, run the search, and also handle other things.

In general, these are the responsibilities of the frontend:
* Parsing user input
* Creating ULP files
* Selecting and calling the backend
* Taking the backend's output and displaying it for the user

And these are those of the backend:
* Running the search
* Reporting solutions
* Duplicate solution removal
* Pre/in-processing
* Reporting long partials
* Reporting progress

## ULP files

This is a unified ini-style format for the specification of problem files for XLS backends.

There are 3 basic sections:
* `[GRAPH]` specifies the abstract clause graph that is the actual problem being solved.
* `[GRID]` specifies how variables in the clause graph map to real patterns in the CA.
* `[OPTIONS]` specifies other options that the backend needs to know.

Whitespace is not significant unless it is being used to separate two alphanumeric values. Any amount or no amount of whitespace can be placed between an alphanumeric value and a symbol, and one or more whitespace characters can be placed between two alphanumberic values. However, newlines are significant, they separate individual lines of the file.

There are two basic entities that can be set in a file, "keys" and "lists". Keys have a single value and are assigned by an equals sign, while lists are followed by a colon, the length of the list, a newline, and then a newline-separated list of values. Boolean keys are provided as either 0 or 1.

Before any of the sections, the `max_states = <integer>` key must be defined to be a positive integer greater than or equal to 2, which is the state count being used. In the format, cells are often needed. `max_states` specifies the format of these integers. Integers from `0` to `max_states - 1` encode that cell state, while positive integers `max_state` and above mean variable `value - max_state + 1`, so `max_state` means variable 1, `max_state + 1` means variable 2, and so on.

### `[GRAPH]` section

This section specifies the abstract clause graph, aka the actual problem being solved. The core data is a list of clauses of n variables; each clause is mapped to a clause function of n variables. The clause functions specify a mapping between data and its satisfiability/unsatisfiability implication (which may itself be a variable; this enables multi-rule searching).

For simple searches in the INT rulespace there is generally only one clause function that maps to the rule, it is of 10 variables specifying the next value of the cell, the cell itself, and its 8 neighbors, returning SAT if the transition is possible and UNSAT if it isn't.

The `var_count = <number>` key must be set, this is the number of variables that the problem uses. This includes the implicit state variables, so it's 2 for a 0-variable problem when `max_states` is 2.

The `funcs` list must be given. Each element of this list is a clause function. Clause functions look like this: `3 lookup_table 0 1 1 0 1 0 0 1`. The first value is the number of variables in the clause function, then the rest specifies the actual function, with the first value in that part being the type. The `rule_func_index = <integer>` key must also be given, this specifies the function used when unparsing the rule.

Clause function types:
* `lookup_table` - A simple lookup table, the data is a `max_states ** func_var_count`-long list of values.

Finally, the `clauses` list must be given. A clause looks like: `0 9 2 0 0 0 0 2 3 0 0 0`. The first number is the number of the clause function to use, the second is the number of variables, and the rest are the cell values of the clause.

### `[GRID]` section

This section specifies how variables in the clause graph map to real patterns in the CA.

These keys must be defined first:
* `width = <number>` - The width of the pattern
* `height = <number>` - The height of the pattern
* `gens = <number>` - The number of generations of the pattern

Then, the `grid` list must be given, it is of length `gens * height` and contains `width`-long space-separated lists of cell values defining the pattern.

### `[OPTIONS]` section

This section specifies other options that the backend needs to know.

Possible options:
* `backend = <string>` - Sets the backend to use, currently the only options are `cnf` and `vls`.
* `search_order_list:` - This list contains the fixed search order (only if the backend supports it), as a list of coordinates like `1 2 3` which specifies the cell at t = 1, x = 2, and y = 3.
* `periodic = (dx, dy)c/(period)` - Provided if a periodic pattern is being searched for, changes duplicate solution detection and early exhaustion stuff.
* `check_early_exhaustion = <boolean>` - Enable early exhaustion checking (like LLSSS `--filters bcaf`), it's disabled by default.
* `after_rule_text = <string>` - Sets text to be displayed after the rule, such as `:T20,20`
* `show_solutions = <boolean>` - Set whether solutions are shown
* `max_solutions = <integer>` - Set the maximum number of solutions to show, it will exit early once it reaches that point

### Example

This search will find the domino in B/S1e:

```
max_states = 2

[GRAPH]
var_count = 4
func_count = 1
clause_count = 12
funcs: 1
10 lookup_table 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 0 1 1 0 1 0 1 0 1 0 1 0 0 1 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 0 1 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 0 1 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0
clauses: 12
0 9 0 0 0 0 0 0 0 0 0 2
0 9 0 0 0 0 0 0 0 0 2 3
0 9 0 0 0 0 0 0 0 2 3 0
0 9 0 0 0 0 0 0 0 3 0 0
0 9 0 0 0 0 0 0 2 0 0 0
0 9 2 0 0 0 0 2 3 0 0 0
0 9 3 0 0 0 2 3 0 0 0 0
0 9 0 0 0 0 3 0 0 0 0 0
0 9 0 0 0 2 0 0 0 0 0 0
0 9 0 0 2 3 0 0 0 0 0 0
0 9 0 2 3 0 0 0 0 0 0 0
0 9 0 3 0 0 0 0 0 0 0 0

[GRID]
width = 2
height = 1
gens = 1
data: 1
2 3

[OPTIONS]
periodic = (0, 0)c/1
```
