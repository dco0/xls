
// defines functions for checking and reporting solutions

#pragma once

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "base.c"
#include "pattern.c"


// PLACEHOLDERS
// TO CHANGE LATER
#define MAX_UNPARSED_RULE_LENGTH 256
extern int get_rule(char* rule, bool use_maxrule);

static void print_p_pretty(const Pattern* p, bool is_solution) {
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
    printf("x = %zu, y = %zu, rule = %s", p->width, p->height, rule);
    // check for alternate printing method
    if (is_solution) {
        bool found = false;
        for (size_t t = 0; t < p->gens; t++) {
            for (size_t y = 0; y < p->height; y++) {
                for (size_t x = 0; x < p->width; x++) {
                    Cell value = p_get(p, t, x, y);
                    if (c_is_var(value)) {
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
            for (size_t y = 0; y < p->height; y++) {
                print_line_padding();
                for (size_t x = 0; x < p->width; x++) {
                    Cell value = p_get(p, 0, x, y);
                    if (value == C_ON) {
                        real_printf("o");
                    } else {
                        real_printf(".");
                    }
                }
                if (y == p->height - 1) {
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
    for (size_t y = 0; y < p->height; y++) {
        print_line_padding();
        for (size_t t = 0; t < p->gens; t++) {
            for (size_t x = 0; x < p->width; x++) {
                Cell value = p_get(p, t, x, y);
                if (value == C_OFF) {
                    real_printf(".");
                } else if (value == C_ON) {
                    real_printf("o");
                } else {
                    real_printf("B");
                }
            }
            if (t != p->gens - 1) {
                real_printf(" .|. ");
            }
        }
        if (y == p->height - 1) {
            real_printf(" !\n");
        } else {
            real_printf(" $\n");
        }
    }
}


typedef uint64_t Hash;
#define PRIhash PRIu64
#define HASH_OFFSET UINT64_C(0xcbf29ce484222325)
#define HASH_PRIME UINT64_C(0x00000100000001b3)
#define MAX_HASH UINT64_MAX

#define update_hash(hash, value) \
    do { \
        (hash) = _Generic((value), \
            int8_t: update_hash_uint8((hash), (uint8_t)(value)), \
            uint8_t: update_hash_uint8((hash), (uint8_t)(value)), \
            int16_t: update_hash_uint16((hash), (uint16_t)(value)), \
            uint16_t: update_hash_uint16((hash), (uint16_t)(value)), \
            int32_t: update_hash_uint32((hash), (uint32_t)(value)), \
            uint32_t: update_hash_uint32((hash), (uint32_t)(value)), \
            int64_t: update_hash_uint64((hash), (uint64_t)(value)), \
            uint64_t: update_hash_uint64((hash), (uint64_t)(value)), \
            char*: update_hash_string((hash), (char*)(uintptr_t)(value)), \
            const char*: update_hash_string((hash), (const char*)(uintptr_t)(value)) \
        ); \
    } while (0)

static inline Hash update_hash_uint8(Hash hash, uint8_t value) {
    hash ^= value;
    hash *= HASH_PRIME;
    return hash;
}

static inline Hash update_hash_uint16(Hash hash, uint16_t value) {
    hash = update_hash_uint8(hash, value & 0xff);
    hash = update_hash_uint8(hash, value >> 8);
    return hash;
}

static inline Hash update_hash_uint32(Hash hash, uint32_t value) {
    hash = update_hash_uint8(hash, value & 0xff);
    hash = update_hash_uint8(hash, (value >> 8) & 0xff);
    hash = update_hash_uint8(hash, (value >> 16) & 0xff);
    hash = update_hash_uint8(hash, value >> 24);
    return hash;
}

static inline Hash update_hash_uint64(Hash hash, uint64_t value) {
    hash = update_hash_uint8(hash, value & 0xff);
    hash = update_hash_uint8(hash, (value >> 8) & 0xff);
    hash = update_hash_uint8(hash, (value >> 16) & 0xff);
    hash = update_hash_uint8(hash, (value >> 24) & 0xff);
    hash = update_hash_uint8(hash, (value >> 32) & 0xff);
    hash = update_hash_uint8(hash, (value >> 40) & 0xff);
    hash = update_hash_uint8(hash, (value >> 48) & 0xff);
    hash = update_hash_uint8(hash, value >> 56);
    return hash;
}

static inline Hash update_hash_string(Hash hash, const char* value) {
    for (size_t i = 0; value[i] != '\0'; i++) {
        update_hash(hash, (int8_t)value[i]);
    }
    return hash;
}

static Hash min_hash(Hash x, Hash y) {
    return x < y ? x : y;
}

static Hash p_hash_at_time(const Pattern* p, size_t t) {
    Hash out = HASH_OFFSET;
    update_hash(out, p->width);
    update_hash(out, p->height);
    for (size_t y = 0; y < p->height; y++) {
        for (size_t x = 0; x < p->width; x++) {
            update_hash(out, p_get(p, t, x, y));
        }
    }
    return out;
}

static Hash p_hash_all_times(const Pattern* p) {
    Hash out = HASH_OFFSET;
    for (size_t t = 0; t < p->gens; t++) {
        for (size_t y = 0; y < p->height; y++) {
            for (size_t x = 0; x < p->width; x++) {
                Cell value = p_get(p, t, x, y);
                update_hash(out, value);
            }
        }
    }
    return out;
}

static Hash p_hash_all_times_periodic_with_offset(const Pattern* p, size_t offset, intmax_t dx, intmax_t dy) {
    Hash out = HASH_OFFSET;
    // determine x_offset_0 and y_offset_0
    Pattern full_t_p = EMPTY_PATTERN;
    Pattern t_p = EMPTY_PATTERN;
    p_extract_gen(&full_t_p, p, offset);
    ShrinkToFitOffsets offsets = p_shrink_to_fit(&t_p, &full_t_p);
    size_t x_offset_0 = offsets.x;
    size_t y_offset_0 = offsets.y;
    for (size_t fake_t = 0; fake_t < config.periodic_period; fake_t++) {
        size_t real_t = (fake_t + offset) % config.periodic_period;
        p_extract_gen(&full_t_p, p, real_t);
        ShrinkToFitOffsets offsets = p_shrink_to_fit(&t_p, &full_t_p);
        intmax_t x_offset = (intmax_t)offsets.x - (intmax_t)x_offset_0;
        intmax_t y_offset = (intmax_t)offsets.y - (intmax_t)y_offset_0;
        if (fake_t > real_t) {
            x_offset += dx;
            y_offset += dy;
        }
        update_hash(out, x_offset);
        update_hash(out, y_offset);
        for (size_t y = 0; y < t_p.height; y++) {
            for (size_t x = 0; x < t_p.width; x++) {
                Cell value = p_get(t_p, 0, x, y);
                update_hash(out, value);
            }
        }
    }
    p_destroy(&full_t_p);
    p_destroy(&t_p);
    return out;
}

static Hash p_hash_all_times_periodic(const Pattern* p, size_t dx, size_t dy) {
    Hash out = MAX_HASH;
    for (size_t offset = 0; offset < config.periodic_period; offset++) {
        out = min_hash(out, p_hash_all_times_periodic_with_offset(p, offset, dx, dy));
    }
    return out;
}

static Hash p_hash_full(const Pattern* p) {
    StaticSymmetry rule_symmetry = get_rule_static_symmetry();
    StaticSymmetry p_symmetry = p_get_static_symmetry(p);
    StaticSymmetry symmetry = STATIC_SYMMETRY_MEET[rule_symmetry][p_symmetry];
    Transformations transforms = static_symmetry_to_transforms(symmetry);
    Pattern temp = EMPTY_PATTERN;
    Hash out;
    if (config.periodic) {
        intmax_t dx = config.periodic_dx;
        intmax_t dy = config.periodic_dy;
        out = p_hash_all_times_periodic(p, dx, dy);
        #define add_hash(transform, dx, dy) \
            if (transforms.transform) { \
                p_##transform(&temp, p); \
                out = min_hash(out, p_hash_all_times_periodic(&temp, (dx), (dy))); \
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
        out = p_hash_all_times(p);
        #define add_hash(transform) \
            if (transforms.transform) { \
                p_##transform(&temp, p); \
                out = min_hash(out, p_hash_all_times(&temp)); \
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
    p_destroy(&temp);
\    return out;
}

