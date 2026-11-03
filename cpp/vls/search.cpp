
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <vector>

#include "../base/base.cpp"
#include "../base/search.cpp"
#include "../base/solutions.cpp"
#include "base.cpp"
#include "implications.cpp"

namespace vls {


std::vector<Variable*> search_order_list;

void get_search_order_list() {
    if (!ulp->options.search_order_list) {
        error("VLS backend requires a search order list");
    }
    search_order_list.clear();
    for (size_t i = 0; i < ulp->options.search_order_list_len; i++) {
        size_t* item = ulp->options.search_order_list[i];
        GridCell cell = ulp->grid.get(item[0], item[1], item[2]);
        Variable* var = ulp->vars[cell];
        search_order_list.push_back(var);
    }
}


static std::vector<int> progress;

void print_progress(FILE* stream) {
    for (const int value : progress) {
        real_fprintf(stream, "%i\n", value);
    }
}


static Grid* max_partial = nullptr;
static 

static void max_partials_end(void) {
    if (state.solutions_found == 0 && config.max_partials) {
        printf("Max partial (score: %ju):\n", max_partial_score);
        print_grid_pretty(&max_partial, false);
    }
    dg_destroy(&max_partial);
}

static void output_checking_function_run_this_on_every_branch(size_t depth) {
    if (config.max_partials) {
        if (state.solutions_found == 0) {
            uintmax_t score;
            if (config.max_partial_scoring == MAX_PARTIAL_SCORING_CELL) {
                score = state.set_unknown_vars;
            } else if (config.max_partial_scoring == MAX_PARTIAL_SCORING_DEPTH) {
                score = depth;
            } else {
                unexpected_error("Invalid max partial scoring: %i", config.max_partial_scoring);
            }
            if (score > max_partial_score) {
                dg_init_from_search_grid(&max_partial);
                max_partial_score = score;
            }
        }
    }
    if (state.branches % CHECK_TIME_EVERY_THIS_MANY_BRANCHES == 0) {
        double time = get_time();
        if (time - last_progress_shown > config.reporting_interval) {
            last_progress_shown = time;
            printf("%ju second%s, %zu branch%s, %zu solution%s, progress: ", plural((uintmax_t)(time - start_time), "s"), plural(state.branches, "es"), plural(state.solutions_found, "s"));
            print_progress(stdout);
            real_printf("\n");
        }
        if (config.max_partials) {
            if (state.solutions_found == 0 && time - last_printed_max_partial_time > config.max_partial_reporting_interval && max_partial_score > last_printed_max_partial_score) {
                last_printed_max_partial_time = time;
                last_printed_max_partial_score = max_partial_score;
                printf("New max partial (score = %"PRIu64"):\n", max_partial_score);
                print_grid_pretty(&max_partial, false);
            }
        }
    }
}


bool all_zeros = true;

// returns true if early exhausted, false if not
bool check_early_exhaustion(void) {
    return false;
    // size_t t = config.start_padding;
    // // check columns
    // if (config.periodic_dx < 0) {
    //     for (size_t x = config.left_padding; x < (size_t)(-config.periodic_dx + 1) + config.left_padding; x++) {
    //         for (size_t y = config.top_padding; y < state.height - config.bottom_padding; y++) {
    //             if (state_get_cell_value(t, x, y) != OFF) {
    //                 return false;
    //             }
    //         }
    //     }
    // } else if (config.periodic_dx > 0) {
    //     for (size_t x = state.width - config.periodic_dx - 1 + config.right_padding; x < state.width - config.right_padding; x++) {
    //         for (size_t y = config.top_padding; y < state.height - config.bottom_padding; y++) {
    //             if (state_get_cell_value(t, x, y) != OFF) {
    //                 return false;
    //             }
    //         }
    //     }
    // } else {
    //     // check left column and right column
    //     bool found = false;
    //     for (size_t y = config.top_padding; y < state.height - config.bottom_padding; y++) {
    //         if (state_get_cell_value(t, config.left_padding, y) != OFF) {
    //             found = true;
    //             break;
    //         }
    //     }
    //     if (!found) {
    //         return true;
    //     }
    //     found = false;
    //     for (size_t y = config.top_padding; y < state.height - config.bottom_padding; y++) {
    //         if (state_get_cell_value(t, state.width - 1 - config.right_padding, y) != OFF) {
    //             found = true;
    //             break;
    //         }
    //     }
    //     if (!found) {
    //         return true;
    //     }
    // }
    // // check rows
    // if (config.periodic_dy < 0) {
    //     for (size_t y = config.top_padding; y < (size_t)(-config.periodic_dy + 1) + config.top_padding; y++) {
    //         for (size_t x = config.left_padding; x < state.width - config.right_padding; x++) {
    //             if (state_get_cell_value(t, x, y) != OFF) {
    //                 return false;
    //             }
    //         }
    //     }
    //     return true;
    // } else if (config.periodic_dy > 0) {
    //     for (size_t y = state.height - config.periodic_dy - 1 - config.bottom_padding; y < state.height - config.bottom_padding; y++) {
    //         for (size_t x = config.left_padding; x < state.width - config.right_padding; x++) {
    //             if (state_get_cell_value(t, x, y) != OFF) {
    //                 return false;
    //             }
    //         }
    //     }
    //     return true;
    // } else {
    //     // check top row and bottom row
    //     bool found = false;
    //     for (size_t x = config.left_padding; x < state.width - config.right_padding; x++) {
    //         if (state_get_cell_value(t, x, config.top_padding) != OFF) {
    //             return false;
    //         }
    //     }
    //     if (!found) {
    //         return true;
    //     }
    //     found = false;
    //     for (size_t x = config.left_padding; x < state.width - config.right_padding; x++) {
    //         if (state_get_cell_value(t, x, state.height - 1 - config.bottom_padding) != OFF) {
    //             return false;
    //         }
    //     }
    //     if (!found) {
    //         return true;
    //     }
    //     return false;
    // }
}

// returns number of iterations to backjump
size_t run_depth(size_t depth, size_t search_order_pos) {
    inc_debug_depth();
    debug(3,
        if (search_order_pos >= search_order_list.size()) {
            printf("Running depth %zu (variable <out of bounds>): ", depth);
        } else {
            printf("Running depth %zu (variable %zu): ", depth, search_order_list[search_order_pos]->number);
        }
        print_progress(stdout);
    );
    search->branches++;
    if (search_order_pos >= search_order_list.size()) {
        solutions::check(*search, false);
        dec_debug_depth();
        return 0;
    }
    output_checking_function_run_this_on_every_branch(depth);
    if (cell->value != UNKNOWN) {
        debug(3, printf("Cell is known, continuing\n"));
        size_t out = run_depth(depth + 1, cell->next_in_search_order);
        dec_debug_depth();
        return out == 0 ? 0 : out - 1;
    }
    CellValue value = config.initial_value;
    for (int i = 0; i < 2; i++) {
        bool prev_all_zeros = all_zeros;
        if (value == ON) {
            all_zeros = false;
        }
        progress[progress_pos] = i;
        progress_pos++;
        debug(3, printf("Attempting to set variable %zu to %i (prev_value = %i)\n", var.number, value, var.value));
        current_stack.push_frame();
        debug(4, current_stack.print());
        size_t out = 0;
        if (set_variable_and_propagate(var, value, true)) {
            // check for early exhaustion
            if (ulp->options.check_early_exhaustion && all_zeros) {
                if (check_early_exhaustion()) {
                    debug(3, printf("Early exhausted\n"));
                    current_stack.pop_and_undo_frame();
                    return 0;
                }
            }
            out = run_depth(depth + 1, search_order_pos);
        }
        current_stack.pop_and_undo_frame();
        progress_pos--;
        if (out != 0) {
            return out - 1;
        }
        all_zeros = prev_all_zeros;
        value = value == OFF ? ON : OFF;
    }
    dec_debug_depth();
    return 0;
}


void search(void) {
    debug(2, printf("Running search\n"));
    state.branches = 0;
    state.solutions_found = 0;
    progress.clear();
    start_time = get_time();
    last_progress_shown = start_time;
    last_printed_max_partial_time = start_time;
    all_zeros = true;
    run_depth(0, initial_cell);
    double time = get_time() - start_time;
    printf("Search complete, found %ju solution%s in %.6f second%s, %ju branch%s\n", plural(state.solutions_found, "s"), plural(time, "s"), plural(state.branches, "es"));
    max_partials_end();
}


}
