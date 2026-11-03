
CNF backend

VLS backend:
* preprocessing: check every assignment of variables in every clause and look for forced assignments or aliases, also remove clauses which can't assign anything
* preprocessing: custom set_cell_and_propagate that also does aliases in `vls::Preprocessor::check_variable_implications`
* preprocessing: run every possible search order up to some depth
* add inprocessing somehow??? requires redesigning ULP to allow copies...

Scripting:
* for mut
* partial ranges
* grid methods (particularly apply_wrap)
* fix number -> state_specifier implicit casting to only allow stat counts
* more builtins
* constrain statement
* iterate over grids (yields coords)
* user-defined functions
* multi-rule
* but first write preprocessor and solution filter and hook it all up after making it usable on the TS side
* string concat and methods
* actually also slap some good idea methods on all the types
* fs and stdin access, so you can write meta search programs as scripts
