
#pragma once

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <optional>
#include <string>

#include "base.cpp"
#include "grid.cpp"
#include "search.cpp"


typedef struct INTSpec {
    int trs_count;
    int max_map_per_tr;
    int number_count;
    int max_letters_per_num;
    const char* letters[9];
    int16_t trs[51][49];
} INTSpec;

const INTSpec normal_int = {
    .trs_count = 51,
    .max_map_per_tr = 8,
    .number_count = 9,
    .max_letters_per_num = 13,
    .letters = {
        "c",
        "ce",
        "aceikn",
        "aceijknqry",
        "aceijknqrtwyz",
        "aceijknqry",
        "aceikn",
        "ce",
        "c",
    },
    .trs = {
        {0, -1},
        {4, 256, 1, 64, -1},
        {2, 128, 8, 32, -1},
        {6, 384, 3, 9, 72, 36, 192, 288, -1},
        {5, 320, 65, 260, -1},
        {34, 160, 10, 136, -1},
        {130, 40, -1},
        {66, 129, 258, 264, 12, 96, 132, 33, -1},
        {68, 257, -1},
        {38, 416, 11, 200, -1},
        {69, 321, 261, 324, -1},
        {42, 168, 138, 162, -1},
        {292, 73, 7, 448, -1},
        {137, 74, 164, 224, 35, 392, 290, 14, -1},
        {98, 161, 266, 140, -1},
        {37, 352, 13, 67, 193, 262, 328, 388, -1},
        {100, 289, 265, 259, 196, 70, 76, 385, -1},
        {131, 194, 134, 104, 41, 296, 386, 44, -1},
        {133, 322, 97, 268, -1},
        {420, 294, 201, 39, 480, 15, 75, 456, -1},
        {325, -1},
        {170, -1},
        {45, 360, 195, 390, -1},
        {169, 106, 172, 226, 163, 394, 298, 142, -1},
        {99, 225, 270, 330, 141, 354, 396, 165, -1},
        {356, 293, 329, 263, 452, 71, 77, 449, -1},
        {102, 417, 267, 204, -1},
        {139, 202, 166, 232, 43, 424, 418, 46, -1},
        {135, 450, 105, 300, -1},
        {228, 291, 393, 78, -1},
        {389, 326, 197, 101, 353, 269, 323, 332, -1},
        {198, 387, 297, 108, -1},
        {457, 79, 484, 295, -1},
        {426, 174, 234, 171, -1},
        {453, 327, 357, 333, -1},
        {203, 422, 488, 47, -1},
        {358, 421, 331, 271, 460, 103, 205, 481, -1},
        {397, 334, 229, 355, -1},
        {458, 143, 482, 428, 302, 233, 167, 107, -1},
        {395, 206, 230, 236, 299, 425, 419, 110, -1},
        {364, 301, 361, 391, 454, 199, 109, 451, -1},
        {362, 173, 398, 227, -1},
        {489, 111, 492, 486, 423, 459, 303, 207, -1},
        {490, 175, 430, 235, -1},
        {461, 335, 485, 359, -1},
        {365, 455, -1},
        {429, 366, 237, 231, 483, 399, 363, 462, -1},
        {427, 238, -1},
        {491, 239, 494, 431, -1},
        {493, 367, 487, 463, -1},
        {495, -1},
    },
};

const INTSpec hex_int = {
    .trs_count = 13,
    .max_map_per_tr = 48,
    .number_count = 7,
    .max_letters_per_num = 3,
    .letters = {
        "o",
        "o",
        "omp",
        "omp",
        "omp",
        "o",
        "o",
    },
    .trs = {
        {0, 4, 64, 68, -1},
        {32, 36, 96, 100, 2, 6, 66, 70, 1, 5, 65, 69, 8, 12, 72, 76, 128, 132, 192, 196, 256, 260, 320, 324, -1},
        {34, 38, 98, 102, 3, 7, 67, 71, 9, 13, 73, 77, 136, 140, 200, 204, 384, 388, 448, 452, 288, 292, 352, 356, -1},
        {33, 37, 97, 101, 10, 14, 74, 78, 129, 133, 193, 197, 264, 268, 328, 332, 160, 164, 224, 228, 258, 262, 322, 326, -1},
        {40, 44, 104, 108, 130, 134, 194, 198, 257, 261, 321, 325, -1},
        {35, 39, 99, 103, 11, 15, 75, 79, 137, 141, 201, 205, 392, 396, 456, 460, 416, 420, 480, 484, 290, 294, 354, 358, -1},
        {42, 46, 106, 110, 41, 45, 105, 109, 162, 166, 226, 230, 131, 135, 195, 199, 168, 172, 232, 236, 138, 142, 202, 206, 289, 293, 353, 357, 259, 263, 323, 327, 296, 300, 360, 364, 265, 269, 329, 333, 386, 390, 450, 454, 385, 389, 449, 453, -1},
        {161, 165, 225, 229, 266, 270, 330, 334, -1},
        {393, 397, 457, 461, 424, 428, 488, 492, 418, 422, 482, 486, 291, 295, 355, 359, 43, 47, 107, 111, 139, 143, 203, 207, -1},
        {394, 398, 458, 462, 417, 421, 481, 485, 298, 302, 362, 366, 163, 167, 227, 231, 267, 271, 331, 335, 169, 173, 233, 237, -1},
        {387, 391, 451, 455, 297, 301, 361, 365, 170, 174, 234, 238, -1},
        {395, 399, 459, 463, 425, 429, 489, 493, 426, 430, 490, 494, 419, 423, 483, 487, 299, 303, 363, 367, 171, 175, 235, 239, -1},
        {427, 431, 491, 495, -1},
    },
};

static void get_trs_from_clause_funcs(int* trs, int unknown_tr_value) {
    const Variable** table = s.get_rule_func();
    for (int i = 0; i < 512; i++) {
        int j = (i & 273) | ((i >> 2) & 34) | ((i >> 4) & 4) | ((i << 2) & 136) | ((i << 4) & 64);
        Cell zero = table[(j << 1) | 0]->value;
        Cell one = table[(j << 1) | 1]->value;
        if (zero == 1 && one == 0) {
            trs[i] = 0;
        } else if (zero == 0 && one == 1) {
            trs[i] = 1;
        } else {
            trs[i] = unknown_tr_value;
        }
    }
}

static std::optional<std::string> unparse_transitions(const int* trs, const INTSpec* spec, bool s) {
    int s_or = s ? (1 << 4) : 0;
    std::string out;
    // stores what index in the transitions we are in
    int trs_index = 0;
    // iterate through each number
    for (int number = 0; number < spec->number_count; number++) {
        // array to hold the letters that we've seen
        char seen_letters[14];
        memset(seen_letters, '\0', 14);
        int num_letters = 0;
        int total_letters = 0;
        for (int i = 0; i < spec->max_letters_per_num + 1; i++) {
            signed char letter = spec->letters[number][i];
            // -1 means that the letters have ran out
            if (letter == -1) {
                break;
            }
            total_letters++;
            // figure out which of the MAP transitions we have
            int count = 0;
            int total = 0;
            for (int i = 0; i < spec->max_map_per_tr; i++) {
                int16_t value = spec->trs[trs_index][i];
                // -1 means that the MAP transitions have ran out
                if (value == -1) {
                    break;
                }
                if (trs[value | s_or]) {
                    count++;
                }
                total++;
            }
            trs_index++;
            // printf("%c%c%c: %i/%i\n", s ? 'S' : 'B', '0' + number, letter, count, total);
            if (count == 0) {
                continue;
            } else if (count == total) {
                seen_letters[num_letters++] = letter;
            } else {
                // it's not in the rulespace
                return std::nullopt;
            }
        }
        // skip if we don't have any of the letters
        if (num_letters == 0) {
            continue;
        }
        // now unparse it
        out += '0' + number;
        if (num_letters == total_letters) {
            continue;
        } else if (num_letters > (total_letters % 2 == 0 ? (total_letters / 2) : (total_letters / 2 + 1))) {
            out += '-';
            for (int i = 0; i < total_letters; i++) {
                char letter = spec->letters[number][i];
                if (!strchr(seen_letters, letter)) {
                    out += letter;
                }
            }
        } else {
            for (int i = 0; i < num_letters; i++) {
                out += seen_letters[i];
            }
        }
    }
}

// attempts to get the full rule using the given spec
static std::optional<std::string> get_int_spec_rule(const int* trs, const INTSpec* spec) {
    std::string out = "B";
    auto b_str = unparse_transitions(trs, spec, false);
    auto s_str = unparse_transitions(trs, spec, true);
    if ((!b_str.has_value() && s_str.has_value())) {
        return std::nullopt;
    }
    return "B" + b_str.value() + "/S" + s_str.value();
}


// format: (x, y)
const int neighborhood_indexing[9][2] = {
    {1, 1},
    {0, 1},
    {-1, 1},
    {1, 0},
    {0, 0},
    {-1, 0},
    {1, -1},
    {0, -1},
    {-1, -1},
};

static void get_trs_neighborhood(bool out[9], const int* trs) {
    for (int i = 0; i < 9; i++) {
        bool found = false;
        for (int tr = 0; tr < 512; tr++) {
            if (trs[(tr & ~(1 << i)) | (0 << i)] != trs[(tr & ~(1 << i)) | (1 << i)]) {
                found = true;
                break;
            }
        }
        out[i] = found;
    }
}

// unparse a general MAP rule
static std::string unparse_map(const int* trs) {
    const static char BASE64_TABLE[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    // get the list of cells that matter
    bool neighborhood[9];
    get_trs_neighborhood(neighborhood, trs);
    // figure out the compactified trs for the right neighborhood
    int type_trs_length;
    int type_trs[512];
    if (neighborhood[0] == false && neighborhood[2] == false && neighborhood[6] == false && neighborhood[8] == false) {
        // von neumann
        type_trs_length = 32;
        for (int i = 0; i < 32; i++) {
            type_trs[i] = trs[((i & 0b10000) << 3) | ((i & 0b1110) << 2) | ((i & 0b1) << 1)];
        }
    } else if (neighborhood[2] == false && neighborhood[6] == false) {
        // hexagonal
        type_trs_length = 128;
        for (int i = 0; i < 128; i++) {
            type_trs[i] = trs[((i & 0b1100000) << 2) | ((i & 0b11100) << 1) | ((i & 0b11) << 0)];
        }
    } else {
        // normal
        type_trs_length = 512;
        memcpy(type_trs, trs, sizeof(type_trs));
    }
    // pack the transitions into bytes
    // we use 66 because it evenly divides into 3
    int unparsed_length = (type_trs_length + 8 - 1) / 8;
    int unparsed[66];
    for (int i = 0; i < 66; i++) {
        unparsed[i] = 0;
    }
    for (int i = 0; i < type_trs_length; i++) {
        if (type_trs[i]) {
            unparsed[i / 8] |= (1 << (7 - (i % 8)));
        }
    }
    // pack the bytes into base64
    std::string out = "MAP";
    #define break_early(used) if (unparsed_length <= i + used) {break;}
    for (int i = 0; i < 66; i += 3) {
        uint32_t value = (unparsed[i] << 16) | (unparsed[i + 1] << 8) | (unparsed[i + 2]);
        out += BASE64_TABLE[(value >> 18) & 0x3f];
        out += BASE64_TABLE[(value >> 12) & 0x3f];
        break_early(1);
        out += BASE64_TABLE[(value >> 6) & 0x3f];
        break_early(2);
        out += BASE64_TABLE[(value >> 0) & 0x3f];
        break_early(3);
    }
    #undef break_early
    return out;
}

static std::string get_rule(bool use_maxrule) {
    int trs[512];
    get_trs_from_clause_funcs(trs, use_maxrule ? 1 : 0);
    std::string out;
    // INT
    auto int_str = get_int_spec_rule(trs, &normal_int);
    if (int_str.has_value()) {
        out = int_str.value();
    } else {
        // hex INT
        auto hex_str = get_int_spec_rule(trs, &hex_int);
        if (hex_str.has_value()) {
            out = hex_str.value() + 'H';
        } else {
            // MAP
            out = unparse_map(trs);
        }
    }
    if (s.options.after_rule_text != NULL) {
        out += s.options.after_rule_text;
    }
    return out;
}

static Transformations get_rule_identity_transforms() {
    Cell func[1024];
    s.get_rule_func_cells(func);
    Transformations out;
    // rotation symmetries
    bool C2 = true;
    bool C4 = true;
    for (int i = 0; i < 512; i++) {
        int j = ((i << 6) & 448) | (i & 56) | (i >> 6);
        j = ((j & 73) << 2) | (j & 146) | ((j & 292) >> 2);
        if ((func[(i << 1) | 0] != func[(j << 1) | 0]) || (func[(i << 1) | 1] != func[(j << 1) | 1])) {
            C2 = false;
            C4 = false;
            break;
        }
    }
    if (C2) {
        for (int i = 0; i < 512; i++) {
            int j = ((i >> 2) & 66) | ((i >> 4) & 8) | ((i >> 6) & 1) | ((i << 2) & 132) | ((i << 6) & 256) | ((i << 4) & 32) | (i & 16);
            if ((func[(i << 1) | 0] != func[(j << 1) | 0]) || (func[(i << 1) | 1] != func[(j << 1) | 1])) {
                C4 = false;
                break;
            }
        }
    }
    out.rotate_180 = C2;
    out.rotate_left = C4;
    out.rotate_right = C4;
    // flip symmetries
    out.flip_horizontal = true;
    out.flip_vertical = true;
    out.flip_diagonal = true;
    out.flip_anti_diagonal = true;
    for (int i = 0; i < 512; i++) {
        int j = ((i & 73) << 2) | (i & 146) | ((i & 292) >> 2);
        if ((func[(i << 1) | 0] != func[(j << 1) | 0]) || (func[(i << 1) | 1] != func[(j << 1) | 1])) {
            out.flip_horizontal = false;
        }
        j = ((i << 6) & 448) | (i & 56) | (i >> 6);
        if ((func[(i << 1) | 0] != func[(j << 1) | 0]) || (func[(i << 1) | 1] != func[(j << 1) | 1])) {
            out.flip_vertical = false;
        }
        j = (i & 273) | ((i >> 2) & 34) | ((i >> 4) & 4) | ((i << 2) & 136) | ((i << 4) & 64);
        if ((func[(i << 1) | 0] != func[(j << 1) | 0]) || (func[(i << 1) | 1] != func[(j << 1) | 1])) {
            out.flip_diagonal = false;
        }
        j = (i & 84) | ((i << 8) & 256) | ((i >> 8) & 1) | ((i >> 4) & 10) | ((i << 4) & 160);
        if ((func[(i << 1) | 0] != func[(j << 1) | 0]) || (func[(i << 1) | 1] != func[(j << 1) | 1])) {
            out.flip_anti_diagonal = false;
        }
    }
    return out;
}

static StaticSymmetry get_rule_static_symmetry() {
    return transforms_to_static_symmetry(get_rule_identity_transforms());
}


static void print_grid_pretty(const Grid& grid, bool is_solution) {
    const char* rule = get_rule(false).c_str();
    if (s.multi_rule) {
        const char* maxrule = get_rule(false).c_str();
        if (strcmp(rule, maxrule) != 0) {
            printf("#C %s to %s\n", rule, maxrule);
        }
    }
    printf("x = %zu, y = %zu, rule = %s", grid.width, grid.height, rule);
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

typedef uint64_t Hash;
#define PRIhash PRIu64
#define HASH_OFFSET UINT64_C(0xcbf29ce484222325)
#define HASH_PRIME UINT64_C(0x00000100000001b3)
#define MAX_HASH UINT64_MAX

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
    hash = update_hash(hash, value >> 56);
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
    StaticSymmetry rule_symmetry = get_rule_static_symmetry();
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


static bool solution_is_empty(const Grid& grid) {
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

static bool solution_is_subperiod(const Grid& grid) {
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

static void check_solution(bool preprocessing) {
    #define drop_solution(msg) \
        debug(2, printf("Dropping solution (" msg ")\n")); \
        if (preprocessing) { \
            printf("Solved in preprocessing, 0 solutions\n"); \
        } \
        return;
    s.sync_search_grid();
    Grid hash_grid = s.grid.copy();
    Grid solution_grid = s.grid.copy();
    if (solution_is_empty(hash_grid)) {
        drop_solution("empty");
    }
    if (solution_is_subperiod(hash_grid)) {
        drop_solution("subperiod");
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
