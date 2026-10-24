
// defines the pattern type for storing and manipulating dynamically allocated patterns

#pragma once

#include <stddef.h>

#include "base.c"



// TODO: better allocation for this stuff

typedef struct Pattern {
    bool used;
    size_t width;
    size_t height;
    size_t gens;
    Cell* data;
} Pattern;

static const Pattern EMPTY_DYNAMIC_GRID = {
    .used = false,
    .width = 0,
    .height = 0,
    .gens = 0,
    .data = NULL,
};

#define p_index(p, t, x, y) (((p)->data)[((((t) * (p)->height) + (y)) * (p)->width) + (x)])
#define direct_p_index(p, t, x, y) (((p).data)[((((t) * (p).height) + (y)) * (p).width) + (x)])

static void p_destroy(Pattern* p) {
    if (p->used) {
        safe_free(p->data);
        p->used = false;
    }
}

static void p_init(Pattern* p, size_t width, size_t height, size_t gens) {
    if (p->used && p->width == width && p->height == height && p->gens == gens) {
        return;
    }
    p_destroy(p);
    p->used = true;
    p->width = width;
    p->height = height;
    p->gens = gens;
    p->data = safe_malloc(width * height * gens * sizeof(Cell));
}

static inline Cell p_get(const Pattern* p, size_t t, size_t x, size_t y) {
    return p_index(p, t, x, y);
}

static inline void p_set(Pattern* p, size_t t, size_t x, size_t y, Cell value) {
    p_index(p, t, x, y) = value;
}


static bool p_eq(const Pattern* x, const Pattern* y) {
    if (x->width != y->width || x->height != y->height || x->gens != y->gens) {
        return false;
    }
    for (size_t t = 0; t < x->gens; t++) {
        for (size_t yi = 0; yi < x->height; yi++) {
            for (size_t xi = 0; xi < x->width; xi++) {
                if (p_index(x, t, xi, yi) != p_index(y, t, xi, yi)) {
                    return false;
                }
            }
        }
    }
    return true;
}

static void p_copy(Pattern* out, Pattern* p) {
    p_init(out, p->width, p->height, p->gens);
    for (size_t t = 0; t < p->gens; t++) {
        for (size_t y = 0; y < p->height; y++) {
            for (size_t x = 0; x < p->width; x++) {
                p_index(out, t, x, y) = p_index(p, t, x, y);
            }
        }
    }
}

static void p_flip_horizontal(Pattern* out, const Pattern* p) {
    p_init(out, p->width, p->height, p->gens);
    for (size_t t = 0; t < p->gens; t++) {
        for (size_t y = 0; y < p->height; y++) {
            for (size_t x = 0; x < p->width; x++) {
                p_index(out, t, p->width - x - 1, y) = p_index(p, t, x, y);
            }
        }
    }
}

static void p_flip_vertical(Pattern* out, const Pattern* p) {
    p_init(out, p->width, p->height, p->gens);
    for (size_t t = 0; t < p->gens; t++) {
        for (size_t y = 0; y < p->height; y++) {
            for (size_t x = 0; x < p->width; x++) {
                p_index(out, t, x, p->height - y - 1) = p_index(p, t, x, y);
            }
        }
    }
}

static void p_rotate_left(Pattern* out, const Pattern* p) {
    p_init(out, p->height, p->width, p->gens);
    for (size_t t = 0; t < p->gens; t++) {
        for (size_t y = 0; y < p->height; y++) {
            for (size_t x = 0; x < p->width; x++) {
                p_index(out, t, p->height - y - 1, x) = p_index(p, t, x, y);
            }
        }
    }
}


static void p_rotate_right(Pattern* out, const Pattern* p) {
    p_init(out, p->height, p->width, p->gens);
    for (size_t t = 0; t < p->gens; t++) {
        for (size_t y = 0; y < p->height; y++) {
            for (size_t x = 0; x < p->width; x++) {
                p_index(out, t, y, p->width - x - 1) = p_index(p, t, x, y);
            }
        }
    }
}

static void p_rotate_180(Pattern* out, const Pattern* p) {
    p_init(out, p->width, p->height, p->gens);
    for (size_t t = 0; t < p->gens; t++) {
        for (size_t y = 0; y < p->height; y++) {
            for (size_t x = 0; x < p->width; x++) {
                p_index(out, t, p->width - x - 1, p->height - y - 1) = p_index(p, t, x, y);
            }
        }
    }
}

static void p_flip_diagonal(Pattern* out, const Pattern* p) {
    p_init(out, p->height, p->width, p->gens);
    for (size_t t = 0; t < p->gens; t++) {
        for (size_t y = 0; y < p->height; y++) {
            for (size_t x = 0; x < p->width; x++) {
                p_index(out, t, y, x) = p_index(p, t, x, y);
            }
        }
    }
}

static void p_flip_anti_diagonal(Pattern* out, const Pattern* p) {
    p_init(out, p->height, p->width, p->gens);
    for (size_t t = 0; t < p->gens; t++) {
        for (size_t y = 0; y < p->height; y++) {
            for (size_t x = 0; x < p->width; x++) {
                p_index(out, t, p->height - y - 1, p->width - x - 1) = p_index(p, t, x, y);
            }
        }
    }
}

static void p_extract_gen(Pattern* out, const Pattern* p, size_t t) {
    p_init(out, p->width, p->height, 1);
    for (size_t y = 0; y < p->height; y++) {
        for (size_t x = 0; x < p->width; x++) {
            p_index(out, 0, x, y) = p_index(p, t, x, y);
        }
    }
}

static void p_shrink(Pattern* out, const Pattern* p, size_t top, size_t bottom, size_t left, size_t right, size_t start, size_t end) {
    size_t width = p->width - left - right;
    size_t height = p->height - top - bottom;
    size_t gens = p->gens - start - end;
    p_init(out, width, height, gens);
    for (size_t t = 0; t < out->gens; t++) {
        for (size_t y = 0; y < out->height; y++) {
            for (size_t x = 0; x < out->width; x++) {
                p_index(out, t, x, y) = p_index(p, t + start, x + left, y + top);
            }
        }
    }
}

typedef struct ShrinkToFitOffsets {
    size_t x;
    size_t y;
} ShrinkToFitOffsets;

static ShrinkToFitOffsets p_shrink_to_fit(Pattern* out, const Pattern* p) {
    size_t shrink_top = 0;
    for (size_t y = 0; y < p->height; y++) {
        bool found = false;
        for (size_t x = 0; x < p->width; x++) {
            for (size_t t = 0; t < p->gens; t++) {
                if (p_get(p, t, x, y) != OFF) {
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
        } else {
            shrink_top++;
        }
    }
    size_t shrink_bottom = 0;
    for (size_t y = p->height; y > 0; y--) {
        bool found = false;
        for (size_t x = 0; x < p->width; x++) {
            for (size_t t = 0; t < p->gens; t++) {
                if (p_get(p, t, x, y - 1) != OFF) {
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
        } else {
            shrink_bottom++;
        }
    }
    size_t shrink_left = 0;
    for (size_t x = 0; x < p->width; x++) {
        bool found = false;
        for (size_t y = 0; y < p->height; y++) {
            for (size_t t = 0; t < p->gens; t++) {
                if (p_get(p, t, x, y) != OFF) {
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
        } else {
            shrink_left++;
        }
    }
    size_t shrink_right = 0;
    for (size_t x = p->width; x > 0; x--) {
        bool found = false;
        for (size_t y = 0; y < p->height; y++) {
            for (size_t t = 0; t < p->gens; t++) {
                if (p_get(p, t, x - 1, y) != OFF) {
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
        } else {
            shrink_right++;
        }
    }
    p_shrink(out, p, shrink_top, shrink_bottom, shrink_left, shrink_right, 0, 0);;
    return (ShrinkToFitOffsets){.x = shrink_left, .y = shrink_top};
}


static Transformations p_get_identity_transforms(const Pattern* p) {
    Transformations out;
    Pattern temp = EMPTY_DYNAMIC_GRID;
    p_flip_horizontal(&temp, p);
    if (p_eq(p, &temp)) {
        out.flip_horizontal = true;
    }
    p_flip_vertical(&temp, p);
    if (p_eq(p, &temp)) {
        out.flip_vertical = true;
    }
    p_rotate_left(&temp, p);
    if (p_eq(p, &temp)) {
        out.rotate_left = true;
    }
    p_rotate_right(&temp, p);
    if (p_eq(p, &temp)) {
        out.rotate_right = true;
    }
    p_rotate_180(&temp, p);
    if (p_eq(p, &temp)) {
        out.rotate_180 = true;
    }
    p_flip_diagonal(&temp, p);
    if (p_eq(p, &temp)) {
        out.flip_diagonal = true;
    }
    p_flip_anti_diagonal(&temp, p);
    if (p_eq(p, &temp)) {
        out.flip_anti_diagonal = true;
    }
    p_destroy(&temp);
    return out;
}

static StaticSymmetry p_get_static_symmetry(const Pattern* p) {
    return transforms_to_static_symmetry(p_get_identity_transforms(p));
}
