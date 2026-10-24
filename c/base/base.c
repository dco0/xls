
// defines core base stuff

#pragma once

#include <stddef.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


// debugging

#define DEBUG_LEVEL 6

#define debug(level, code) if (DEBUG_LEVEL >= (level)) { code ; }

#define INDENT "    "

#define real_printf (printf)
#define real_fprintf (fprintf)
#if DEBUG_LEVEL >= 2
    static int debug_depth = 0;
    #define inc_debug_depth() do { debug_depth++; } while (0)
    #define dec_debug_depth() do { debug_depth--; } while (0)
    #define print_line_padding() { \
        for (int i = 0; i < debug_depth; i++) { \
            real_printf(INDENT); \
        } \
    }
    #define fprint_line_padding(stream) { \
        for (int i = 0; i < debug_depth; i++) { \
            real_fprintf(stream, INDENT); \
        } \
    }
    #define printf(...) { \
        for (int i = 0; i < debug_depth; i++) { \
            real_printf(INDENT); \
        } \
        real_printf(__VA_ARGS__); \
    }
    #define fprintf(stream, ...) { \
        for (int i = 0; i < debug_depth; i++) { \
            real_fprintf(stream, INDENT); \
        } \
        real_fprintf(stream, __VA_ARGS__); \
    }
#else
    #define inc_debug_depth()
    #define dec_debug_depth()
    #define print_line_padding()
    #define fprint_line_padding(stream)
#endif

// diagnostics

#define error(format, ...) \
    do { \
        real_fprintf(stderr, "Error: "format"\n    at %s:%i\n" __VA_OPT__(,) __VA_ARGS__, __FILE__, __LINE__); \
        exit(EXIT_FAILURE); \
    } while (0)

#define unexpected_error(format, ...) \
    do { \
        real_fprintf(stderr, "Unexpected error: "format"\n    at %s:%i\n" __VA_OPT__(,) __VA_ARGS__, __FILE__, __LINE__); \
        exit(EXIT_FAILURE); \
    } while (0)

// better allocation functions

static void* safe_malloc(size_t size) {
    void* out = malloc(size);
    if (out == NULL) {
        error("In malloc(): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return out;
}

static void* safe_realloc(void* ptr, size_t size) {
    void* out = realloc(ptr, size);
    if (out == NULL) {
        error("In realloc(): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return out;
}

static void safe_free(void* ptr) {
    free(ptr);
}


// core type definitions

typedef uint64_t Cell;
#define OFF 0
#define ON 1
#define is_var(cell) ((cell) > 1)


// symmetries and symmetry transformations

typedef enum StaticSymmetry {
    C1,
    C2,
    C4,
    D2h,
    D2v,
    D2b,
    D2s,
    D4p,
    D4x,
    D8,
} StaticSymmetry;

typedef struct Transformations {
    bool flip_horizontal: 1;
    bool flip_vertical: 1;
    bool rotate_left: 1;
    bool rotate_right: 1;
    bool rotate_180: 1;
    bool flip_diagonal: 1;
    bool flip_anti_diagonal: 1;
} Transformations;

static const StaticSymmetry STATIC_SYMMETRY_JOIN[10][10] = {
    [C1 ] = {C1 , C2 , C4 , D2h, D2v, D2b, D2s, D4p, D4x, D8 },
    [C2 ] = {C2 , C2 , C4 , D4p, D4p, D4x, D4x, D4p, D4x, D8 },
    [C4 ] = {C4 , C4 , C4 , D8 , D8 , D8 , D8 , D8 , D8 , D8 },
    [D2h] = {D2h, D4p, D8 , D2h, D4p, D8 , D8 , D4p, D8 , D8 },
    [D2v] = {D2v, D4p, D8 , D4p, D2v, D8 , D8 , D4p, D8 , D8 },
    [D2b] = {D2b, D4x, D8 , D8 , D8 , D2b, D4x, D8 , D4x, D8 },
    [D2s] = {D2s, D4x, D8 , D8 , D8 , D4x, D2s, D8 , D4x, D8 },
    [D4p] = {D4p, D4p, D8 , D4p, D4p, D8 , D8 , D4p, D8 , D8 },
    [D4x] = {D4x, D4x, D8 , D8 , D8 , D4x, D4x, D8 , D4x, D8 },
    [D8 ] = {D8 , D8 , D8 , D8 , D8 , D8 , D8 , D8 , D8 , D8 },
};

static const StaticSymmetry STATIC_SYMMETRY_MEET[10][10] = {
    [C1 ] = {C1 , C1 , C1 , C1 , C1 , C1 , C1 , C1 , C1 , C1 },
    [C2 ] = {C1 , C2 , C2 , C1 , C1 , C1 , C1 , C2 , C2 , C2 },
    [C4 ] = {C1 , C2 , C4 , C1 , C1 , C1 , C1 , C2 , C2 , C4 },
    [D2h] = {C1 , C1 , C1 , D2h, C1 , C1 , C1 , D2h, C1 , D2h},
    [D2v] = {C1 , C1 , C1 , C1 , D2v, C1 , C1 , D2v, C1 , D2v},
    [D2b] = {C1 , C1 , C1 , C1 , C1 , D2b, C1 , C1 , D2b, D2b},
    [D2s] = {C1 , C1 , C1 , C1 , C1 , C1 , D2s, C1 , D2s, D2s},
    [D4p] = {C1 , C2 , C2 , D2h, D2v, C1 , C1 , D4p, C2 , D4p},
    [D4x] = {C1 , C2 , C2 , C1 , C1 , D2b, D2s, C2 , D4x, D4x},
    [D8 ] = {C1 , C2 , C4 , D2h, D2v, D2b, D2s, D4p, D4x, D8 },
};

static bool static_symmetry_contains(StaticSymmetry container, StaticSymmetry value) {
    return STATIC_SYMMETRY_JOIN[container][value] == container;
}

static Transformations static_symmetry_to_transforms(StaticSymmetry symmetry) {
    Transformations out;
    out.flip_horizontal = static_symmetry_contains(symmetry, D2h);
    out.flip_vertical = static_symmetry_contains(symmetry, D2v);
    out.rotate_left = static_symmetry_contains(symmetry, C4);
    out.rotate_right = static_symmetry_contains(symmetry, C4);
    out.rotate_180 = static_symmetry_contains(symmetry, C2);
    out.flip_diagonal = static_symmetry_contains(symmetry, D2b);
    out.flip_anti_diagonal = static_symmetry_contains(symmetry, D2s);
    return out;
}

static StaticSymmetry transforms_to_static_symmetry(Transformations t) {
    bool iC2 = t.rotate_180;
    bool iC4 = t.rotate_left || t.rotate_right;
    bool iD2h = t.flip_horizontal;
    bool iD2v = t.flip_vertical;
    bool iD2b = t.flip_diagonal;
    bool iD2s = t.flip_anti_diagonal;
    if ((iD2h || iD2v) && (iD2b || iD2s)) {
        return D8;
    } else if (iC2) {
        if (iC4) {
            if (iD2h || iD2v || iD2s || iD2b) {
                return D8;
            } else {
                return C4;
            }
        } else {
            if (iD2h || iD2v) {
                return D4p;
            } else if (iD2b || iD2s) {
                return D4x;
            } else {
                return C2;
            }
        }
    } else {
        if (iD2h && iD2v) {
            return D4p;
        } else if (iD2b && iD2s) {
            return D4x;
        } else if (iD2h) {
            return D2h;
        } else if (iD2v) {
            return D2v;
        } else if (iD2s) {
            return D2s;
        } else if (iD2b) {
            return D2b;
        } else {
            return C1;
        }
    }
}
