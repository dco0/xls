
#pragma once

#include <limits.h>
#include <stddef.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <unordered_map>

#include "params.cpp"
#include "base.cpp"


class Grid {

private:

    inline static std::unordered_map<size_t, std::vector<Cell*>> free_lists;
    inline static std::unordered_map<size_t, size_t> allocation_counts;
    inline static size_t current_allocated_bytes = 0UZ;

    static void clean_cache() {
        // clean it to reasonably below the limit to avoid constant calling of this function
        size_t target_bytes = GRID_CACHE_MAX_MEMORY * 0.75;
        // gather all non-empty cache sizes and their frequencies
        struct Candidate {
            size_t size;
            size_t freq;
        };
        std::vector<Candidate> candidates;
        for (const auto& [cached_size, list] : free_lists) {
            if (!list.empty()) {
                candidates.push_back({cached_size, allocation_counts[cached_size]});
            }
        }
        // sort by frequency ascending (least frequently used first)
        std::sort(candidates.begin(), candidates.end(), 
            [](const auto& a, const auto& b) {
                return a.freq < b.freq;
            }
        );
        // evict blocks from cold sizes until we are safely below the target watermark
        for (const auto& c : candidates) {
            auto& list = free_lists[c.size];
            while (!list.empty() && current_allocated_bytes > target_bytes) {
                Cell* ptr = list.back();
                list.pop_back();
                safe_free(ptr);
                current_allocated_bytes -= c.size;
            }
            if (current_allocated_bytes <= target_bytes) {
                break;
            }
            if (list.empty()) {
                free_lists.erase(c.size);
            }
        }
    }

    static Cell* allocate_cells(size_t size) {
        allocation_counts[size]++;
        auto& list = free_lists[size];
        if (!list.empty()) {
            Cell* ptr = list.back();
            list.pop_back();
            return ptr;
        } else {
            current_allocated_bytes += size;
            if (current_allocated_bytes > GRID_CACHE_MAX_MEMORY) {

            }
            return (Cell*)safe_malloc(size * sizeof(Cell));
        }
    }

    static void deallocate_cells(Cell* ptr, size_t size) {
        if (ptr == nullptr) {
            return;
        }
        auto& list = free_lists[size];
        size_t freq = allocation_counts[size];
        size_t current_max_size = std::min(GRID_CACHE_MAX_SIZE, GRID_CACHE_BASE_SIZE + (size_t)((double)freq / GRID_CACHE_SIZE_SCALING));
        if (list.size() < current_max_size) {
            list.push_back(ptr);
        } else {
            current_allocated_bytes -= size;
            safe_free(ptr);
        }
    }

public:

    static void destroy() {
        for (const auto& [_, list] : free_lists) {
            for (Cell* ptr : list) {
                safe_free(ptr);
            }
        }
        free_lists.clear();
        allocation_counts.clear();
    }

    size_t width;
    size_t height;
    size_t gens;
    size_t total_size;
    Cell* data;

    Grid(size_t width, size_t height, size_t gens) {
        this->width = width;
        this->height = height;
        this->gens = gens;
        total_size = width * height * gens;
        data = allocate_cells(total_size);
    }

    ~Grid() {
        deallocate_cells(data, total_size);
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
