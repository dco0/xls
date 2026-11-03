
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

#include "base.cpp"
#include "grid.cpp"
#include "ulp.cpp"
#include "rule.cpp"


inline void print_grid_cell_pretty(const ULP& ulp, GridCell value) {
    const char* RLE_CHARS = ".ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if (ulp.max_states >= 3) {
        if (!ulp.gc_is_var(value)) {
            real_printf("%c", RLE_CHARS[value]);
        } else {
            real_printf("%c", RLE_CHARS[ulp.max_states]);
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
    std::string rule = rule::get(ulp, false);
    if (ulp.multi_rule) {
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
                    if (ulp.gc_is_var(grid.get(t, x, y))) {
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
                    GridCell value = grid.get(0, x, y);
                    print_grid_cell_pretty(ulp, value);
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
                GridCell value = grid.get(t, x, y);
                print_grid_cell_pretty(ulp, value);
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
    Options& options;

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

    Search(Options& options) : ulp(options), options(options) {
        // do nothing
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
        if (options.show_max_partials) {
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
