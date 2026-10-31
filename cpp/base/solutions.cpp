
#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <memory>
#include <optional>
#include <string>

#include "base.cpp"
#include "grid.cpp"
#include "params.hpp"
#include "search.cpp"
#include "rule.cpp"


static void print_grid_pretty(const Grid& grid, bool is_solution) {
    std::string rule = rule::get(false);
    if (s.multi_rule) {
        std::string maxrule = rule::get(true);
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
                    if (c_is_var(grid.get(t, x, y))) {
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
                    Cell value = grid.get(0, x, y);
                    if (value == C_ON) {
                        real_printf("o");
                    } else {
                        real_printf(".");
                    }
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
                Cell value = grid.get(t, x, y);
                if (value == C_OFF) {
                    real_printf(".");
                } else if (value == C_ON) {
                    real_printf("o");
                } else {
                    real_printf("B");
                }
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


namespace solutions {


typedef uint64_t Hash;
#define PRIhash PRIu64
constexpr Hash HASH_OFFSET = 0xcbf29ce484222325ULL;
constexpr Hash HASH_PRIME = 0x00000100000001b3ULL;
constexpr Hash MAX_HASH = UINT64_MAX;

static inline Hash update_hash(Hash hash, uint8_t value) {
    hash ^= value;
    hash *= HASH_PRIME;
    return hash;
}

static inline Hash update_hash(Hash hash, int8_t value) {
    return update_hash(hash, (uint8_t)(value));
}

static inline Hash update_hash(Hash hash, uint16_t value) {
    hash = update_hash(hash, (uint8_t)(value & 0xff));
    hash = update_hash(hash, (uint8_t)(value >> 8));
    return hash;
}

static inline Hash update_hash(Hash hash, int16_t value) {
    return update_hash(hash, (uint16_t)(value));
}

static inline Hash update_hash(Hash hash, uint32_t value) {
    hash = update_hash(hash, (uint8_t)(value & 0xff));
    hash = update_hash(hash, (uint8_t)((value >> 8) & 0xff));
    hash = update_hash(hash, (uint8_t)((value >> 16) & 0xff));
    hash = update_hash(hash, (uint8_t)(value >> 24));
    return hash;
}

static inline Hash update_hash(Hash hash, int32_t value) {
    return update_hash(hash, (uint32_t)(value));
}

static inline Hash update_hash(Hash hash, uint64_t value) {
    hash = update_hash(hash, (uint8_t)(value & 0xff));
    hash = update_hash(hash, (uint8_t)((value >> 8) & 0xff));
    hash = update_hash(hash, (uint8_t)((value >> 16) & 0xff));
    hash = update_hash(hash, (uint8_t)((value >> 24) & 0xff));
    hash = update_hash(hash, (uint8_t)((value >> 32) & 0xff));
    hash = update_hash(hash, (uint8_t)((value >> 40) & 0xff));
    hash = update_hash(hash, (uint8_t)((value >> 48) & 0xff));
    hash = update_hash(hash, (uint8_t)(value >> 56));
    return hash;
}

static inline Hash update_hash(Hash hash, int64_t value) {
    return update_hash(hash, (uint64_t)(value));
}

static inline Hash update_hash(Hash hash, const char* value) {
    for (size_t i = 0; value[i] != '\0'; i++) {
        update_hash(hash, value[i]);
    }
    return hash;
}

static Hash min_hash(Hash x, Hash y) {
    return x < y ? x : y;
}

// static Hash grid_hash_at_time(const Grid& grid, size_t t) {
//     Hash out = HASH_OFFSET;
//     update_hash(out, grid.width);
//     update_hash(out, grid.height);
//     for (size_t y = 0; y < grid.height; y++) {
//         for (size_t x = 0; x < grid.width; x++) {
//             update_hash(out, grid.get(t, x, y));
//         }
//     }
//     return out;
// }

static Hash grid_hash_all_times(const Grid& grid) {
    Hash out = HASH_OFFSET;
    for (size_t t = 0; t < grid.gens; t++) {
        for (size_t y = 0; y < grid.height; y++) {
            for (size_t x = 0; x < grid.width; x++) {
                Cell value = grid.get(t, x, y);
                update_hash(out, value);
            }
        }
    }
    return out;
}

static Hash grid_hash_all_times_periodic_with_offset(const Grid& grid, size_t offset, intmax_t dx, intmax_t dy) {
    Hash out = HASH_OFFSET;
    // determine x_offset_0 and y_offset_0
    const auto offsets = grid.extract_gen(offset).get_shrink_to_fit_offsets();
    size_t x_offset_0 = offsets.left;
    size_t y_offset_0 = offsets.top;
    for (size_t fake_t = 0; fake_t < s.options.periodic_period; fake_t++) {
        size_t real_t = (fake_t + offset) % s.options.periodic_period;
        const Grid t_grid = grid.extract_gen(real_t);
        const auto offsets = t_grid.get_shrink_to_fit_offsets();
        size_t shrink_width = t_grid.width - offsets.left - offsets.right;
        size_t shrink_height = t_grid.height - offsets.top - offsets.bottom;
        intmax_t x_offset = (intmax_t)offsets.left - (intmax_t)x_offset_0;
        intmax_t y_offset = (intmax_t)offsets.top - (intmax_t)y_offset_0;
        if (fake_t > real_t) {
            x_offset += dx;
            y_offset += dy;
        }
        update_hash(out, x_offset);
        update_hash(out, y_offset);
        for (size_t y = 0; y < shrink_height; y++) {
            for (size_t x = 0; x < shrink_width; x++) {
                Cell value = t_grid.get(0, x + offsets.left, y + offsets.top);
                update_hash(out, value);
            }
        }
    }
    return out;
}

static Hash grid_hash_all_times_periodic(const Grid& grid, size_t dx, size_t dy) {
    Hash out = MAX_HASH;
    for (size_t offset = 0; offset < s.options.periodic_period; offset++) {
        out = min_hash(out, grid_hash_all_times_periodic_with_offset(grid, offset, dx, dy));
    }
    return out;
}

static Hash grid_hash_full(const Grid& grid) {
    StaticSymmetry rule_symmetry = rule::get_static_symmetry();
    StaticSymmetry grid_symmetry = grid.get_static_symmetry();
    StaticSymmetry symmetry = STATIC_SYMMETRY_MEET[rule_symmetry][grid_symmetry];
    Transformations transforms = static_symmetry_to_transforms(symmetry);
    Hash out;
    if (s.options.periodic) {
        intmax_t dx = s.options.periodic_dx;
        intmax_t dy = s.options.periodic_dy;
        out = grid_hash_all_times_periodic(grid, dx, dy);
        #define add_hash(transform, dx, dy) \
            if (transforms.transform) { \
                out = min_hash(out, grid_hash_all_times_periodic(grid.transform(), (dx), (dy))); \
            }
        add_hash(flip_horizontal, -dx, dy);
        add_hash(flip_vertical, dx, -dy);
        add_hash(rotate_left, -dy, dx);
        add_hash(rotate_right, dy, -dx);
        add_hash(rotate_180, -dx, -dy);
        add_hash(flip_diagonal, dy, dx);
        add_hash(flip_anti_diagonal, -dy, -dx);
        #undef add_hash
    } else {
        out = grid_hash_all_times(grid);
        #define add_hash(transform) \
            if (transforms.transform) { \
                out = min_hash(out, grid_hash_all_times(grid.transform())); \
            }
        add_hash(flip_horizontal);
        add_hash(flip_vertical);
        add_hash(rotate_left);
        add_hash(rotate_right);
        add_hash(rotate_180);
        add_hash(flip_diagonal);
        add_hash(flip_anti_diagonal);
        #undef add_hash
    }
    return out;
}


static std::unique_ptr<Hash> prev_solution_hashes = nullptr;

void init() {
    prev_solution_hashes = std::make_unique<Hash>(MAX_SAVED_SOLUTION_HASHES);
}


static bool is_empty(const Grid& grid) {
    for (size_t t = 0; t < grid.gens; t++) {
        for (size_t y = 0; y < grid.height; y++) {
            for (size_t x = 0; x < grid.width; x++) {
                if (grid.get(t, x, y) != V_OFF) {
                    return false;
                }
            }
        }
    }
    return true;
}

static bool is_subperiod(const Grid& grid) {
    if (!s.options.periodic) {
        return false;
    }
    const Grid gen_0 = grid.extract_gen(0);
    for (size_t i = 1; i < s.options.periodic_period; i++) {
        if (gen_0.eq(grid.extract_gen(i))) {
            return true;
        }
    }
    return false;
}

static void check(bool preprocessing, bool (*check_duplicate)(Grid& grid)) {
    #define drop_solution(msg) \
        debug(2, printf("Dropping solution (" msg ")\n")); \
        if (preprocessing) { \
            printf("Solved in preprocessing, 0 solutions\n"); \
        } \
        return;
    s.sync_search_grid();
    Grid hash_grid = s.grid.copy();
    Grid solution_grid = s.grid.copy();
    if (is_empty(hash_grid)) {
        drop_solution("empty");
    }
    if (is_subperiod(hash_grid)) {
        drop_solution("subperiod");
    }
    if (check_duplicate != nullptr) {
        if (!check_duplicate(hash_grid)) {
            drop_solution("duplicate");
        }
    }
    // show the solution
    search_data.solutions_found++;
    if (s.options.show_solutions) {
        if (preprocessing) {
            printf("Solved in preprocessing, 1 solution:\n");
        } else {
            printf("Solution found:\n");
        }
        print_grid_pretty(solution_grid, true);
    }
    if (s.options.max_solutions > 0) {
        if (search_data.solutions_found >= s.options.max_solutions) {
            double time = get_time() - search_data.start_time;
            printf("Search complete, found %zu solution%s in %.6f second%s, %zu branch%s (exited early, max solution count reached)\n", plural(search_data.solutions_found, "s"), plural(time, "s"), plural(search_data.branches, "es"));
            destroy_all();
            exit(EXIT_SUCCESS);
        }
    }
}


}
