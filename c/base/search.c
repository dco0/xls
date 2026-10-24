
// defines types for holding and loading the search from ULP

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "base.c"
#include "pattern.c"


typedef int VarValue;
#define V_UNKNOWN 0
#define V_OFF 1
#define V_ON 2

typedef struct Clause Clause;

typedef struct VarUse {
    Clause* clause;
    // for VLS backend trust me it needs it for the preprocessor too
    int64_t change;
} VarUse;

typedef struct Variable {
    size_t number;
    VarValue value;
    size_t use_count;
    VarUse uses[];
} Variable;

typedef struct Clause {
    size_t number;
    bool removed;
    ClauseFunc* func;
    // for VLS backend trust me it needs it for the preprocessor too
    uint64_t* implication_pointer;
    size_t var_count;
    Variable* vars[];
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


static void normalize_search_grid(void) {
    for (size_t t = 0; t < s.grid.gens; t++) {
        for (size_t y = 0; y < s.grid.height; y++) {
            for (size_t x = 0; x < s.grid.width; x++) {
                Cell cell = p_get(s.grid, t, x, y);
            }
        }
    }
}
