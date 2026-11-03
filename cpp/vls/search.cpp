
#pragma once

#include <stddef.h>
#include <stdio.h>
#include <vector>

#include "../base/base.cpp"
#include "../base/search.cpp"
#include "base.cpp"
#include "implications.cpp"

namespace vls {


static std::vector<Variable*> get_search_order_list() {
    if (!ulp->options.search_order_list) {
        error("VLS backend requires a search order list");
    }
    std::vector<Variable*> out;
    for (size_t i = 0; i < ulp->options.search_order_list_len; i++) {
        size_t* item = ulp->options.search_order_list[i];
        Cell cell = ulp->grid.get(item[0], item[1], item[2]);
        Variable* var = ulp->vars[cell];
        out.push_back(var);
    }
    return out;
}


static std::vector<int> progress;

static void print_progress() {
    
}


bool all_zeros = true;

// returns true if early exhausted, false if not
static bool check_early_exhaustion(void) {
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
static size_t run_depth(size_t depth, size_t search_order_pos) {
    inc_debug_depth();
    debug(3,
        printf("Running depth %zu: ", depth);
        print_progress(stdout);
        real_printf("\n");
        if (cell == NULL) {
            printf("Cell: <null pointer>\n");
        } else {
            printf("Cell: variable = %zu\n", cell->var_number);
        }
    );
    search->branches++;
    // add 2 to account for off-by-one errors
    if (depth > ulp->options.search_order_list_len + 2) {
        unexpected_error("The dwarves delved too greedily and too deep");
    }
    if (search.set_unknown_vars >= search.start_unknown_vars || cell == NULL) {
        check_solution(false);
        dec_debug_depth();
        return 0;
    }
    debug(3, print_grid(stdout));
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


static void search(void) {
    Cell* initial_cell = add_search_orders();
    debug(2, printf("Running search\n"));
    state.branches = 0;
    state.solutions_found = 0;
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
