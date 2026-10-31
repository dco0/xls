
#pragma once

#include <stddef.h>

#include "base.cpp"


class Grid { public:

    size_t width;
    size_t height;
    size_t gens;
    size_t total_size;
    Cell* data;

    Grid(size_t initial_width, size_t initial_height, size_t initial_gens) {
        width = initial_width;
        height = initial_height;
        gens = initial_gens;
        total_size = width * height * gens;
        data = (Cell*)safe_malloc(total_size * sizeof(Cell));
    }

    ~Grid() {
        safe_free(data);
    }

    Cell get(size_t t, size_t x, size_t y) const {
        return data[((t * height) + y) * width + x];
    }

    void set(size_t t, size_t x, size_t y, Cell value) {
        data[((t * height) + y) * width + x] = value;
    }

    bool eq(const Grid& other) const {
        if (width != other.width || height != other.height || gens != other.gens) {
            return false;
        }
        for (size_t i = 0; i < total_size; i++) {
            if (data[i] != other.data[i]) {
                return false;
            }
        }
        return true;
    }

    Grid copy() const {
        Grid out = Grid(width, height, gens);
        memcpy(out.data, data, total_size * sizeof(Cell));
        return out;
    }

    Grid flip_horizontal() const {
        Grid out = Grid(width, height, gens);
        for (size_t t = 0; t < gens; t++) {
            for (size_t y = 0; y < height; y++) {
                for (size_t x = 0; x < width; x++) {
                    out.set(t, width - x - 1, y, get(t, x, y));
                }
            }
        }
        return out;
    }

    Grid flip_vertical() const {
        Grid out = Grid(width, height, gens);
        for (size_t t = 0; t < gens; t++) {
            for (size_t y = 0; y < height; y++) {
                for (size_t x = 0; x < width; x++) {
                    out.set(t, x, height - y - 1, get(t, x, y));
                }
            }
        }
        return out;
    }

    Grid rotate_left() const {
        Grid out = Grid(height, width, gens);
        for (size_t t = 0; t < gens; t++) {
            for (size_t y = 0; y < width; y++) {
                for (size_t x = 0; x < height; x++) {
                    out.set(t, height - y - 1, x, get(t, x, y));
                }
            }
        }
        return out;
    }

    Grid rotate_right() const {
        Grid out = Grid(height, width, gens);
        for (size_t t = 0; t < gens; t++) {
            for (size_t y = 0; y < width; y++) {
                for (size_t x = 0; x < height; x++) {
                    out.set(t, y, width - x - 1, get(t, x, y));
                }
            }
        }
        return out;
    }

    Grid rotate_180() const {
        Grid out = Grid(width, height, gens);
        for (size_t t = 0; t < gens; t++) {
            for (size_t y = 0; y < height; y++) {
                for (size_t x = 0; x < width; x++) {
                    out.set(t, width - x - 1, height - y - 1, get(t, x, y));
                }
            }
        }
        return out;
    }

    Grid flip_diagonal() const {
        Grid out = Grid(height, width, gens);
        for (size_t t = 0; t < gens; t++) {
            for (size_t y = 0; y < width; y++) {
                for (size_t x = 0; x < height; x++) {
                    out.set(t, y, x, get(t, x, y));
                }
            }
        }
        return out;
    }

    Grid flip_anti_diagonal() const {
        Grid out = Grid(height, width, gens);
        for (size_t t = 0; t < gens; t++) {
            for (size_t y = 0; y < width; y++) {
                for (size_t x = 0; x < height; x++) {
                    out.set(t, height - y - 1, width - x - 1, get(t, x, y));
                }
            }
        }
        return out;
    }

    Grid extract_gen(size_t t) const {
        Grid out = Grid(width, height, 1);
        for (size_t y = 0; y < height; y++) {
            for (size_t x = 0; x < width; x++) {
                out.set(0, x, y, get(t, x, y));
            }
        }
        return out;
    }

    Grid shrink(size_t start, size_t end, size_t left, size_t right, size_t top, size_t bottom) const {
        size_t new_width = width - left - right;
        size_t new_height = height - top - bottom;
        size_t new_gens = gens - start - end;
        Grid out = Grid(new_width, new_height, new_gens);
        for (size_t t = 0; t < gens; t++) {
            for (size_t y = 0; y < height; y++) {
                for (size_t x = 0; x < width; x++) {
                    out.set(t, x, y, get(t + start, x + left, y + top));
                }
            }
        }
        return out;
    }

    struct ShrinkToFitOffsets {
        size_t top;
        size_t bottom;
        size_t left;
        size_t right;
    };

    ShrinkToFitOffsets get_shrink_to_fit_offsets() const {
        size_t top = 0;
        for (size_t y = 0; y < height; y++) {
            bool found = false;
            for (size_t x = 0; x < width; x++) {
                for (size_t t = 0; t < gens; t++) {
                    if (get(t, x, y) != C_OFF) {
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
                top++;
            }
        }
        size_t bottom = 0;
        for (size_t y = height; y > 0; y--) {
            bool found = false;
            for (size_t x = 0; x < width; x++) {
                for (size_t t = 0; t < gens; t++) {
                    if (get(t, x, y - 1) != C_OFF) {
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
                bottom++;
            }
        }
        size_t left = 0;
        for (size_t x = 0; x < width; x++) {
            bool found = false;
            for (size_t y = 0; y < height; y++) {
                for (size_t t = 0; t < gens; t++) {
                    if (get(t, x, y) != C_OFF) {
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
                left++;
            }
        }
        size_t right = 0;
        for (size_t x = width; x > 0; x--) {
            bool found = false;
            for (size_t y = 0; y < height; y++) {
                for (size_t t = 0; t < gens; t++) {
                    if (get(t, x - 1, y) != C_OFF) {
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
                right++;
            }
        }
        ShrinkToFitOffsets out = {.top = top, .bottom = bottom, .left = left, .right = right};
        return out;
    }

    Grid shrink_to_fit() {
        ShrinkToFitOffsets offsets = get_shrink_to_fit_offsets();
        Grid out = shrink(0, 0, offsets.left, offsets.right, offsets.top, offsets.bottom);
        return out;
    }

    Transformations get_identity_transforms() const {
        Transformations out;
        if (eq(flip_horizontal())) {
            out.flip_horizontal = true;
        }
        if (eq(flip_vertical())) {
            out.flip_vertical = true;
        }
        if (eq(rotate_left())) {
            out.rotate_left = true;
        }
        if (eq(rotate_right())) {
            out.rotate_right = true;
        }
        if (eq(rotate_180())) {
            out.rotate_180 = true;
        }
        if (eq(flip_diagonal())) {
            out.flip_diagonal = true;
        }
        if (eq(flip_anti_diagonal())) {
            out.flip_anti_diagonal = true;
        }
        return out;
    }

    StaticSymmetry get_static_symmetry() const {
        return transforms_to_static_symmetry(get_identity_transforms());
    }

};
