
#pragma once

#include <stddef.h>

#include "base.cpp"


// TODO: better allocation for this stuff

typedef struct Grid {
    bool used;
    size_t width;
    size_t height;
    size_t gens;
    Cell* data;
} Grid;

static const Grid EMPTY_GRID = {
    .used = false,
    .width = 0,
    .height = 0,
    .gens = 0,
    .data = NULL,
};

#define gget(grid, t, x, y) (grid)->data[((((t) * (grid)->height) + (y)) * (grid)->width) + (x)]

static void grid_destroy(Grid* grid) {
    if (grid->used) {
        safe_free(grid->data);
        grid->used = false;
    }
}

static void grid_init(Grid* grid, size_t width, size_t height, size_t gens) {
    if (grid->used && grid->width == width && grid->height == height && grid->gens == gens) {
        return;
    }
    grid_destroy(grid);
    grid->used = true;
    grid->width = width;
    grid->height = height;
    grid->gens = gens;
    grid->data = (Cell*) safe_malloc(width * height * gens * sizeof(Cell));
}


static bool grid_eq(const Grid* x, const Grid* y) {
    if (x->width != y->width || x->height != y->height || x->gens != y->gens) {
        return false;
    }
    for (size_t t = 0; t < x->gens; t++) {
        for (size_t yi = 0; yi < x->height; yi++) {
            for (size_t xi = 0; xi < x->width; xi++) {
                if (gget(x, t, xi, yi) != gget(y, t, xi, yi)) {
                    return false;
                }
            }
        }
    }
    return true;
}

static void grid_copy(Grid* out, Grid* grid) {
    grid_init(out, grid->width, grid->height, grid->gens);
    for (size_t t = 0; t < grid->gens; t++) {
        for (size_t y = 0; y < grid->height; y++) {
            for (size_t x = 0; x < grid->width; x++) {
                gget(out, t, x, y) = gget(grid, t, x, y);
            }
        }
    }
}

static void grid_flip_horizontal(Grid* out, const Grid* grid) {
    grid_init(out, grid->width, grid->height, grid->gens);
    for (size_t t = 0; t < grid->gens; t++) {
        for (size_t y = 0; y < grid->height; y++) {
            for (size_t x = 0; x < grid->width; x++) {
                gget(out, t, grid->width - x - 1, y) = gget(grid, t, x, y);
            }
        }
    }
}

static void grid_flip_vertical(Grid* out, const Grid* grid) {
    grid_init(out, grid->width, grid->height, grid->gens);
    for (size_t t = 0; t < grid->gens; t++) {
        for (size_t y = 0; y < grid->height; y++) {
            for (size_t x = 0; x < grid->width; x++) {
                gget(out, t, x, grid->height - y - 1) = gget(grid, t, x, y);
            }
        }
    }
}

static void grid_rotate_left(Grid* out, const Grid* grid) {
    grid_init(out, grid->height, grid->width, grid->gens);
    for (size_t t = 0; t < grid->gens; t++) {
        for (size_t y = 0; y < grid->height; y++) {
            for (size_t x = 0; x < grid->width; x++) {
                gget(out, t, grid->height - y - 1, x) = gget(grid, t, x, y);
            }
        }
    }
}

static void grid_rotate_right(Grid* out, const Grid* grid) {
    grid_init(out, grid->height, grid->width, grid->gens);
    for (size_t t = 0; t < grid->gens; t++) {
        for (size_t y = 0; y < grid->height; y++) {
            for (size_t x = 0; x < grid->width; x++) {
                gget(out, t, y, grid->width - x - 1) = gget(grid, t, x, y);
            }
        }
    }
}

static void grid_rotate_180(Grid* out, const Grid* grid) {
    grid_init(out, grid->width, grid->height, grid->gens);
    for (size_t t = 0; t < grid->gens; t++) {
        for (size_t y = 0; y < grid->height; y++) {
            for (size_t x = 0; x < grid->width; x++) {
                gget(out, t, grid->width - x - 1, grid->height - y - 1) = gget(grid, t, x, y);
            }
        }
    }
}

static void grid_flip_diagonal(Grid* out, const Grid* grid) {
    grid_init(out, grid->height, grid->width, grid->gens);
    for (size_t t = 0; t < grid->gens; t++) {
        for (size_t y = 0; y < grid->height; y++) {
            for (size_t x = 0; x < grid->width; x++) {
                gget(out, t, y, x) = gget(grid, t, x, y);
            }
        }
    }
}

static void grid_flip_anti_diagonal(Grid* out, const Grid* grid) {
    grid_init(out, grid->height, grid->width, grid->gens);
    for (size_t t = 0; t < grid->gens; t++) {
        for (size_t y = 0; y < grid->height; y++) {
            for (size_t x = 0; x < grid->width; x++) {
                gget(out, t, grid->height - y - 1, grid->width - x - 1) = gget(grid, t, x, y);
            }
        }
    }
}

static void grid_extract_gen(Grid* out, const Grid* grid, size_t t) {
    grid_init(out, grid->width, grid->height, 1);
    for (size_t y = 0; y < grid->height; y++) {
        for (size_t x = 0; x < grid->width; x++) {
            gget(out, 0, x, y) = gget(grid, t, x, y);
        }
    }
}

static void grid_shrink(Grid* out, const Grid* grid, size_t top, size_t bottom, size_t left, size_t right, size_t start, size_t end) {
    size_t width = grid->width - left - right;
    size_t height = grid->height - top - bottom;
    size_t gens = grid->gens - start - end;
    grid_init(out, width, height, gens);
    for (size_t t = 0; t < out->gens; t++) {
        for (size_t y = 0; y < out->height; y++) {
            for (size_t x = 0; x < out->width; x++) {
                gget(out, t, x, y) = gget(grid, t + start, x + left, y + top);
            }
        }
    }
}

typedef struct ShrinkToFitOffsets {
    size_t x;
    size_t y;
} ShrinkToFitOffsets;

static ShrinkToFitOffsets grid_shrink_to_fit(Grid* out, const Grid* grid) {
    size_t shrink_top = 0;
    for (size_t y = 0; y < grid->height; y++) {
        bool found = false;
        for (size_t x = 0; x < grid->width; x++) {
            for (size_t t = 0; t < grid->gens; t++) {
                if (gget(grid, t, x, y) != C_OFF) {
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
    for (size_t y = grid->height; y > 0; y--) {
        bool found = false;
        for (size_t x = 0; x < grid->width; x++) {
            for (size_t t = 0; t < grid->gens; t++) {
                if (gget(grid, t, x, y - 1) != C_OFF) {
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
    for (size_t x = 0; x < grid->width; x++) {
        bool found = false;
        for (size_t y = 0; y < grid->height; y++) {
            for (size_t t = 0; t < grid->gens; t++) {
                if (gget(grid, t, x, y) != C_OFF) {
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
    for (size_t x = grid->width; x > 0; x--) {
        bool found = false;
        for (size_t y = 0; y < grid->height; y++) {
            for (size_t t = 0; t < grid->gens; t++) {
                if (gget(grid, t, x - 1, y) != C_OFF) {
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
    grid_shrink(out, grid, shrink_top, shrink_bottom, shrink_left, shrink_right, 0, 0);
    ShrinkToFitOffsets offsets = {.x = shrink_left, .y = shrink_top};
    return offsets;
}


static Transformations grid_get_identity_transforms(const Grid* grid) {
    Transformations out;
    Grid temp = EMPTY_GRID;
    grid_flip_horizontal(&temp, grid);
    if (grid_eq(grid, &temp)) {
        out.flip_horizontal = true;
    }
    grid_flip_vertical(&temp, grid);
    if (grid_eq(grid, &temp)) {
        out.flip_vertical = true;
    }
    grid_rotate_left(&temp, grid);
    if (grid_eq(grid, &temp)) {
        out.rotate_left = true;
    }
    grid_rotate_right(&temp, grid);
    if (grid_eq(grid, &temp)) {
        out.rotate_right = true;
    }
    grid_rotate_180(&temp, grid);
    if (grid_eq(grid, &temp)) {
        out.rotate_180 = true;
    }
    grid_flip_diagonal(&temp, grid);
    if (grid_eq(grid, &temp)) {
        out.flip_diagonal = true;
    }
    grid_flip_anti_diagonal(&temp, grid);
    if (grid_eq(grid, &temp)) {
        out.flip_anti_diagonal = true;
    }
    grid_destroy(&temp);
    return out;
}

static StaticSymmetry grid_get_static_symmetry(const Grid* g) {
    return transforms_to_static_symmetry(grid_get_identity_transforms(g));
}
