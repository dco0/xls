
#pragma once

#include <stddef.h>

#include "base.c"


// TODO: better allocation for this stuff

typedef struct Grid {
    bool used;
    size_t width;
    size_t height;
    size_t gens;
    Cell* data;
} Grid;

static const Grid EMPTY_PATTERN = {
    .used = false,
    .width = 0,
    .height = 0,
    .gens = 0,
    .data = NULL,
};

#define g_get(g, t, x, y) (g)->data[((((t) * (g)->height) + (y)) * (g)->width) + (x)]

static void g_destroy(Grid* g) {
    if (g->used) {
        safe_free(g->data);
        g->used = false;
    }
}

static void g_init(Grid* g, size_t width, size_t height, size_t gens) {
    if (g->used && g->width == width && g->height == height && g->gens == gens) {
        return;
    }
    g_destroy(g);
    g->used = true;
    g->width = width;
    g->height = height;
    g->gens = gens;
    g->data = (Cell*) safe_malloc(width * height * gens * sizeof(Cell));
}


static bool g_eq(const Grid* x, const Grid* y) {
    if (x->width != y->width || x->height != y->height || x->gens != y->gens) {
        return false;
    }
    for (size_t t = 0; t < x->gens; t++) {
        for (size_t yi = 0; yi < x->height; yi++) {
            for (size_t xi = 0; xi < x->width; xi++) {
                if (g_get(x, t, xi, yi) != g_get(y, t, xi, yi)) {
                    return false;
                }
            }
        }
    }
    return true;
}

static void g_copy(Grid* out, Grid* g) {
    g_init(out, g->width, g->height, g->gens);
    for (size_t t = 0; t < g->gens; t++) {
        for (size_t y = 0; y < g->height; y++) {
            for (size_t x = 0; x < g->width; x++) {
                g_get(out, t, x, y) = g_get(g, t, x, y);
            }
        }
    }
}

static void g_flig_horizontal(Grid* out, const Grid* g) {
    g_init(out, g->width, g->height, g->gens);
    for (size_t t = 0; t < g->gens; t++) {
        for (size_t y = 0; y < g->height; y++) {
            for (size_t x = 0; x < g->width; x++) {
                g_get(out, t, g->width - x - 1, y) = g_get(g, t, x, y);
            }
        }
    }
}

static void g_flig_vertical(Grid* out, const Grid* g) {
    g_init(out, g->width, g->height, g->gens);
    for (size_t t = 0; t < g->gens; t++) {
        for (size_t y = 0; y < g->height; y++) {
            for (size_t x = 0; x < g->width; x++) {
                g_get(out, t, x, g->height - y - 1) = g_get(g, t, x, y);
            }
        }
    }
}

static void g_rotate_left(Grid* out, const Grid* g) {
    g_init(out, g->height, g->width, g->gens);
    for (size_t t = 0; t < g->gens; t++) {
        for (size_t y = 0; y < g->height; y++) {
            for (size_t x = 0; x < g->width; x++) {
                g_get(out, t, g->height - y - 1, x) = g_get(g, t, x, y);
            }
        }
    }
}


static void g_rotate_right(Grid* out, const Grid* g) {
    g_init(out, g->height, g->width, g->gens);
    for (size_t t = 0; t < g->gens; t++) {
        for (size_t y = 0; y < g->height; y++) {
            for (size_t x = 0; x < g->width; x++) {
                g_get(out, t, y, g->width - x - 1) = g_get(g, t, x, y);
            }
        }
    }
}

static void g_rotate_180(Grid* out, const Grid* g) {
    g_init(out, g->width, g->height, g->gens);
    for (size_t t = 0; t < g->gens; t++) {
        for (size_t y = 0; y < g->height; y++) {
            for (size_t x = 0; x < g->width; x++) {
                g_get(out, t, g->width - x - 1, g->height - y - 1) = g_get(g, t, x, y);
            }
        }
    }
}

static void g_flig_diagonal(Grid* out, const Grid* g) {
    g_init(out, g->height, g->width, g->gens);
    for (size_t t = 0; t < g->gens; t++) {
        for (size_t y = 0; y < g->height; y++) {
            for (size_t x = 0; x < g->width; x++) {
                g_get(out, t, y, x) = g_get(g, t, x, y);
            }
        }
    }
}

static void g_flig_anti_diagonal(Grid* out, const Grid* g) {
    g_init(out, g->height, g->width, g->gens);
    for (size_t t = 0; t < g->gens; t++) {
        for (size_t y = 0; y < g->height; y++) {
            for (size_t x = 0; x < g->width; x++) {
                g_get(out, t, g->height - y - 1, g->width - x - 1) = g_get(g, t, x, y);
            }
        }
    }
}

static void g_extract_gen(Grid* out, const Grid* g, size_t t) {
    g_init(out, g->width, g->height, 1);
    for (size_t y = 0; y < g->height; y++) {
        for (size_t x = 0; x < g->width; x++) {
            g_get(out, 0, x, y) = g_get(g, t, x, y);
        }
    }
}

static void g_shrink(Grid* out, const Grid* g, size_t top, size_t bottom, size_t left, size_t right, size_t start, size_t end) {
    size_t width = g->width - left - right;
    size_t height = g->height - top - bottom;
    size_t gens = g->gens - start - end;
    g_init(out, width, height, gens);
    for (size_t t = 0; t < out->gens; t++) {
        for (size_t y = 0; y < out->height; y++) {
            for (size_t x = 0; x < out->width; x++) {
                g_get(out, t, x, y) = g_get(g, t + start, x + left, y + top);
            }
        }
    }
}

typedef struct ShrinkToFitOffsets {
    size_t x;
    size_t y;
} ShrinkToFitOffsets;

static ShrinkToFitOffsets g_shrink_to_fit(Grid* out, const Grid* g) {
    size_t shrink_top = 0;
    for (size_t y = 0; y < g->height; y++) {
        bool found = false;
        for (size_t x = 0; x < g->width; x++) {
            for (size_t t = 0; t < g->gens; t++) {
                if (g_get(g, t, x, y) != C_OFF) {
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
    for (size_t y = g->height; y > 0; y--) {
        bool found = false;
        for (size_t x = 0; x < g->width; x++) {
            for (size_t t = 0; t < g->gens; t++) {
                if (g_get(g, t, x, y - 1) != C_OFF) {
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
    for (size_t x = 0; x < g->width; x++) {
        bool found = false;
        for (size_t y = 0; y < g->height; y++) {
            for (size_t t = 0; t < g->gens; t++) {
                if (g_get(g, t, x, y) != C_OFF) {
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
    for (size_t x = g->width; x > 0; x--) {
        bool found = false;
        for (size_t y = 0; y < g->height; y++) {
            for (size_t t = 0; t < g->gens; t++) {
                if (g_get(g, t, x - 1, y) != C_OFF) {
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
    g_shrink(out, g, shrink_top, shrink_bottom, shrink_left, shrink_right, 0, 0);;
    return (ShrinkToFitOffsets){.x = shrink_left, .y = shrink_top};
}


static Transformations g_get_identity_transforms(const Grid* g) {
    Transformations out;
    Grid temp = EMPTY_PATTERN;
    g_flig_horizontal(&temp, g);
    if (g_eq(g, &temp)) {
        out.flip_horizontal = true;
    }
    g_flig_vertical(&temp, g);
    if (g_eq(g, &temp)) {
        out.flip_vertical = true;
    }
    g_rotate_left(&temp, g);
    if (g_eq(g, &temp)) {
        out.rotate_left = true;
    }
    g_rotate_right(&temp, g);
    if (g_eq(g, &temp)) {
        out.rotate_right = true;
    }
    g_rotate_180(&temp, g);
    if (g_eq(g, &temp)) {
        out.rotate_180 = true;
    }
    g_flig_diagonal(&temp, g);
    if (g_eq(g, &temp)) {
        out.flip_diagonal = true;
    }
    g_flig_anti_diagonal(&temp, g);
    if (g_eq(g, &temp)) {
        out.flip_anti_diagonal = true;
    }
    g_destroy(&temp);
    return out;
}

static StaticSymmetry g_get_static_symmetry(const Grid* g) {
    return transforms_to_static_symmetry(g_get_identity_transforms(g));
}
