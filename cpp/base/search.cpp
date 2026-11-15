
#pragma once

#include <cstdlib>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
// sometimes it's not defined, so we have to do this
extern int nanosleep(const struct timespec *__requested_time, struct timespec *__remaining);
#include <string>

#include "params.cpp"
#include "base.cpp"
#include "grid.cpp"
#include "ulp.cpp"
#include "rule.cpp"


inline void print_grid_cell_pretty(const Graph& graph, GridCell value) {
    static const char* RLE_CHARS = ".ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if (graph.max_states >= 3) {
        if (!graph.gc_is_var(value)) {
            real_printf("%c", RLE_CHARS[value]);
        } else {
            real_printf("%c", RLE_CHARS[graph.max_states]);
        }
    } else {
        if (value == 0) {
            real_printf(".");
        } else if (value == 1) {
            real_printf("o");
        } else {
            real_printf("B");
        }
    }
}

void print_grid_pretty(const ULP& ulp, const Grid& grid, bool is_solution) {
    const Graph& graph = *ulp.graph;
    std::string rule = rule::get(ulp, false);
    if (ulp.graph->is_multi_rule()) {
        std::string maxrule = rule::get(ulp, true);
        if (rule != maxrule) {
            printf("#C %s to %s\n", rule.c_str(), maxrule.c_str());
        }
    }
    printf("x = %zu, y = %zu, rule = %s", grid.width, grid.height, rule.c_str());
    // check for alternate printing method
    if (is_solution) {
        bool found = false;
        for (size_t t = 0; t < grid.gens; t++) {
            for (size_t y = 0; y < grid.height; y++) {
                for (size_t x = 0; x < grid.width; x++) {
                    if (graph.gc_is_var(grid[t, x, y])) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    break;
                }
            }
            if (found) {
                break;
            }
        }
        if (!found) {
            // finish the RLE header
            real_printf("\n");
            for (size_t y = 0; y < grid.height; y++) {
                print_line_padding();
                for (size_t x = 0; x < grid.width; x++) {
                    GridCell value = grid[0, x, y];
                    print_grid_cell_pretty(graph, value);
                }
                if (y == grid.height - 1) {
                    real_printf(" !\n");
                } else {
                    real_printf(" $\n");
                }
            }
            return;
        }
    }
    // finish the RLE header
    real_printf("History\n");
    for (size_t y = 0; y < grid.height; y++) {
        print_line_padding();
        for (size_t t = 0; t < grid.gens; t++) {
            for (size_t x = 0; x < grid.width; x++) {
                GridCell value = grid[t, x, y];
                print_grid_cell_pretty(graph, value);
            }
            if (t != grid.gens - 1) {
                real_printf(" .|. ");
            }
        }
        if (y == grid.height - 1) {
            real_printf(" !\n");
        } else {
            real_printf(" $\n");
        }
    }
}


struct Search {

    ULP ulp;
    Options options;

    size_t start_unknown_vars = 0;
    size_t set_unknown_vars = 0;

    uintmax_t solutions_found = 0;
    uintmax_t branches = 0;

    Grid max_partial = Grid(0, 0, 0);
    uintmax_t max_partial_score = 0;
    uintmax_t last_printed_max_partial_score = 0;

    double start_time = 0;
    double last_progress_shown_time = 0;
    double last_max_partial_shown_time = 0;

    Search(Graph* graph) : ulp(graph, options) {
        // do nothing
    }

private:

    // the out buffer should be filled with zeros
    void var_to_string(char* out, Variable& var) {
        VarValue value = var.value;
        if (value == VV_UNKNOWN) {
            size_t use_count = 0;
            for (size_t i = 0; i < ulp.grid.total_size; i++) {
                if (ulp.grid.data[i] == var.number) {
                    use_count++;
                    if (use_count > 1) {
                        break;
                    }
                }
            }
            #if PRINT_GRID_ALWAYS_SHOW_VARS
                sprintf(out, "%zu", var.number);
            #else
                if (use_count > 1) {
                    sprintf(out, "%zu", var->number);
                } else {
                    out[0] = '*';
                }
            #endif
        } else {
            out[0] = ".oBCDEFGHIJKLMNOPQRSTUVWXYZ"[value - 1];
            out[1] = '\0';
        }
    }

public:

    // print the grid for debugging purposes
    void print_grid(FILE* stream) {
        Grid& grid = ulp.grid;
        char str[256];
        int max_len = 1;
        for (size_t t = 0; t < grid.gens; t++) {
            for (size_t y = 0; y < grid.height; y++) {
                for (size_t x = 0; x < grid.width; x++) {
                    Variable* var = ulp.graph->vars[grid[t, x, y]];
                    var_to_string(str, *var);
                    int len = strlen(str);
                    if (len > max_len) {
                        max_len = len;
                    }
                }
            }
        }
        int field_size = max_len + 1;
        std::string rule = rule::get(ulp, false);
        fprintf(stream, "Grid (rule = %s, set_unknown_vars = %zu out of %zu):\n", rule.c_str(), set_unknown_vars, start_unknown_vars);
        for (size_t t = 0; t < grid.gens; t++) {
            for (size_t y = 0; y < grid.height; y++) {
                fprint_line_padding(stream);
                for (size_t x = 0; x < grid.width; x++) {
                    memset(str, '\0', 256);
                    Variable* var = ulp.graph->vars[grid[t, x, y]];
                    var_to_string(str, *var);
                    real_fprintf(stream, "%-*s", field_size, str);
                }
                real_fprintf(stream, "$\n");
            }
            if (t == grid.gens - 1) {
                fprintf(stream, "!\n");
            } else {
                fprintf(stream, "$ %zub\n", t + 1);
            }
        }
    }

    inline void check_for_new_max_partial(size_t depth) {
        if (solutions_found == 0 && options.show_max_partials) {
            uintmax_t score = depth;
            if (score > max_partial_score) {
                max_partial = ulp.grid;
                max_partial_score = score;
            }
        }
    }

    inline void check_for_print_max_partial(double time) {
        if (options.show_max_partials) {
            if (solutions_found == 0 && time - last_max_partial_shown_time > options.max_partial_reporting_interval && max_partial_score > last_printed_max_partial_score) {
                last_progress_shown_time = time;
                last_printed_max_partial_score = max_partial_score;
                printf("New max partial (score = %ju):\n", max_partial_score);
                print_grid_pretty(ulp, max_partial, false);
            }
        }
    }

    void print_max_partial_at_end() {
        if (options.show_max_partials && solutions_found == 0) {
            printf("Max partial (score: %ju):\n", max_partial_score);
            print_grid_pretty(ulp, max_partial, false);
        }
    }

};


#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #ifdef _MSC_VER
        #include <intrin.h>
    #else
        #include <x86intrin.h>
    #endif
    uint64_t read_counter() {
        return __rdtsc();
    }
#elif defined(__aarch64__)
    uint64_t read_counter() {
        uint64_t val;
        __asm__ volatile("mrs %0, cntvct_el0" : "=r" (val));
        return val;
    }
#elif defined(__riscv) && __riscv_xlen == 64
    uint64_t read_counter() {
        uint64_t cycles;
        __asm__ volatile("rdcycle %0" : "=r"(cycles));
        return cycles;
    }
#elif defined(__powerpc__) || defined(__ppc__)
    uint64_t read_counter() {
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

void calibrate_time() {
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

double get_time() {
    return (double)(read_counter()) / cycles_per_second;
}
