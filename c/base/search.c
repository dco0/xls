
// defines types for holding and loading the search from ULP

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "params.h"
#include "pattern.c"


typedef struct Clause {
    Cell* cells[MAX_IMPLIC_FUNC_LENGTH];
    size_t number;
    bool removed;
    uint64_t* implication_pointer;
} Clause;


static struct {

    uint64_t max_states;

    struct {
        uint64_t var_count;
        size_t func_count;
        ClauseFunc* funcs;
        size_t clause_count;
        Clause* clauses;
    } graph;

    Pattern grid;

    struct {
        
    } options;

} s;
