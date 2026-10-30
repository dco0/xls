
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <time.h>
// sometimes it's not defined, so we have to do this
extern int nanosleep(const struct timespec *__requested_time, struct timespec *__remaining);
#include <optional>

#include "base.cpp"
#include "grid.cpp"


struct Variable;

struct LookupTableClauseFunc {
    Variable* table[];
};

enum ClauseFuncType {
    CFT_LOOKUP_TABLE,
};

struct ClauseFunc {
    size_t vars;
    int type;
    union {
        LookupTableClauseFunc lookup_table;
    } data;
};

enum VarValue {
    V_UNKNOWN = 0,
    V_OFF = 1,
    V_ON = 2,
};

#define c_to_v(cell) ((cell) + 1)
#define v_to_c(var) ((var) - 1)

struct Clause;

struct VarUse {
    Clause* clause;
    // for VLS backend trust me it needs it for the preprocessor too
    ptrdiff_t change;
};
 
struct Variable {
    size_t number;
    VarValue value;
    size_t use_count;
    VarUse uses[];
};

struct Clause {
    size_t number;
    bool removed;
    ClauseFunc* func;
    // for VLS backend trust me it needs it for the preprocessor too
    uint64_t* implication_pointer;
    size_t var_count;
    Variable* vars[];
};


struct ULP {

    bool multi_rule;

    uint64_t max_states;

    struct {
        size_t var_count;
        Variable** vars;
        size_t func_count;
        ClauseFunc** funcs;
        size_t rule_func_index;
        size_t clause_count;
        Clause** clauses;
    } graph;

    Grid grid = Grid(0, 0, 0);

    struct {

        bool periodic;
        uintmax_t periodic_dx;
        uintmax_t periodic_dy;
        uintmax_t periodic_period;

        char* after_rule_text;

        bool show_solutions;
        std::optional<uintmax_t> max_solutions;

    } options;

    const Variable** get_rule_func() {
        ClauseFunc* func = graph.funcs[graph.rule_func_index];
        if (func->vars != 10) {
            error("Rule function must have 10 inputs");
        }
        if (func->type != CFT_LOOKUP_TABLE) {
            error("Rule function must be a lookup table");
        }
        return const_cast<const Variable**>(func->data.lookup_table.table);
    }

    void get_rule_func_cells(Cell* out) {
        const Variable** func = get_rule_func();
        for (size_t i = 0; i < 1024; i++) {
            out[i] = func[i]->value;
        }
    }

    void sync_search_grid() {
        for (size_t t = 0; t < grid.gens; t++) {
            for (size_t y = 0; y < grid.height; y++) {
                for (size_t x = 0; x < grid.width; x++) {
                    Cell value = grid.get(t, x, y);
                    if (c_is_var(value)) {
                        Variable* var = graph.vars[value];
                        if (var->value != V_UNKNOWN) {
                            grid.set(t, x, y, v_to_c(var->value));
                        }
                    }
                }
            }
        }
    }

};

static ULP s;


struct {
    uintmax_t solutions_found;
    uintmax_t branches;
    double start_time;
} search_data;

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #ifdef _MSC_VER
        #include <intrin.h>
    #else
        #include <x86intrin.h>
    #endif
    static uint64_t read_counter() {
        return __rdtsc();
    }
#elif defined(__aarch64__)
    static uint64_t read_counter() {
        uint64_t val;
        __asm__ volatile("mrs %0, cntvct_el0" : "=r" (val));
        return val;
    }
#elif defined(__riscv) && __riscv_xlen == 64
    static uint64_t read_counter() {
        uint64_t cycles;
        __asm__ volatile("rdcycle %0" : "=r"(cycles));
        return cycles;
    }
#elif defined(__powerpc__) || defined(__ppc__)
    static uint64_t read_counter() {
        uint64_t tb;
        __asm__ volatile("mfspr %0, 268" : "=r"(tb));
        return tb;
    }
#else
    static uint64_t read_counter() {
        return __builtin_readcyclecounter();
    }
#endif


static double cycles_per_second;

static void calibrate_time() {
    struct timespec request;
    struct timespec remainder;
    // 0.01 seconds
    request.tv_sec = 0;
    request.tv_nsec = 10000000L;
    uint64_t start = read_counter();
    nanosleep(&request, &remainder);
    uint64_t end = read_counter();
    cycles_per_second = (end - start) * 100;
}

static double get_time() {
    return (double)(read_counter()) / cycles_per_second;
}

// cleans up the search state
void destroy_all();
