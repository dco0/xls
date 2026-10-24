
// defines functions for checking and reporting solutions

#pragma once

#include <stdio.h>
#include <string.h>

#include "base.c"
#include "pattern.c"


// PLACEHOLDERS
// TO CHANGE LATER
#define MAX_UNPARSED_RULE_LENGTH 256
extern int get_rule(char* rule, bool use_maxrule);

static void print_grid_pretty(const Pattern* grid, bool is_solution) {
    char rule[MAX_UNPARSED_RULE_LENGTH];
    memset(rule, '\0', 256);
    get_rule(rule, false);
    if (state.multi_rule) {
        char maxrule[MAX_UNPARSED_RULE_LENGTH];
        memset(maxrule, '\0', 256);
        get_rule(maxrule, true);
        if (strcmp(rule, maxrule) != 0) {
            printf("#C %s to %s\n", rule, maxrule);
        }
    }
    printf("x = %zu, y = %zu, rule = %s", grid->width, grid->height, rule);
    // check for alternate printing method
    if (is_solution) {
        bool found = false;
        for (size_t t = 0; t < grid->gens; t++) {
            for (size_t y = 0; y < grid->height; y++) {
                for (size_t x = 0; x < grid->width; x++) {
                    Cell value = p_get(grid, t, x, y);
                    if (is_var(value)) {
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
            for (size_t y = 0; y < grid->height; y++) {
                print_line_padding();
                for (size_t x = 0; x < grid->width; x++) {
                    Cell value = p_get(grid, 0, x, y);
                    if (value == ON) {
                        real_printf("o");
                    } else {
                        real_printf(".");
                    }
                }
                if (y == grid->height - 1) {
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
    for (size_t y = 0; y < grid->height; y++) {
        print_line_padding();
        for (size_t t = 0; t < grid->gens; t++) {
            for (size_t x = 0; x < grid->width; x++) {
                Cell value = p_get(grid, t, x, y);
                if (value == OFF) {
                    real_printf(".");
                } else if (value == ON) {
                    real_printf("o");
                } else {
                    real_printf("B");
                }
            }
            if (t != grid->gens - 1) {
                real_printf(" .|. ");
            }
        }
        if (y == grid->height - 1) {
            real_printf(" !\n");
        } else {
            real_printf(" $\n");
        }
    }
}

