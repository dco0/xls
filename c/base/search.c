
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "base.c"
#include "grid.c"


typedef struct LookupTableClauseFunc {
    size_t vars;
    Cell data[];
} LookupTableClauseFunc;

typedef enum ClauseFuncType {
    CFT_LOOKUP_TABLE,
} ClauseFuncType;

typedef struct ClauseFunc {
    int type;
    union {
        LookupTableClauseFunc lookup_table;
    } data;
} ClauseFunc;

typedef enum VarValue {
    V_UNKNOWN = 0,
    V_OFF = 1,
    V_ON = 2,
} VarValue;

#define c_to_v(cell) ((cell) + 1)
#define v_to_c(var) ((var) - 1)

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


typedef struct ULP {

    bool multi_rule;

    uint64_t max_states;

    struct {
        size_t var_count;
        Variable** vars;
        size_t func_count;
        ClauseFunc** funcs;
        size_t clause_count;
        Clause** clauses;
    } graph;

    Grid grid;

    struct {

        bool periodic;
        uintmax_t periodic_dx;
        uintmax_t periodic_dy;
        uintmax_t periodic_period;

    } options;

} ULP;

static ULP s;


static void s_sync_search_grid(void) {
    for (size_t t = 0; t < s.grid.gens; t++) {
        for (size_t y = 0; y < s.grid.height; y++) {
            for (size_t x = 0; x < s.grid.width; x++) {
                Cell value = g_get(&s.grid, t, x, y);
                if (c_is_var(value)) {
                    Variable* var = s.graph.vars[value];
                    if (var->value != V_UNKNOWN) {
                        g_get(&s.grid, t, x, y) = v_to_c(var->value);
                    }
                }
            }
        }
    }
}
