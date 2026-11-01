
#pragma once

#include <cstdlib>
#include <stddef.h>
#include <errno.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
// sometimes it's not defined, so we have to do this
extern int nanosleep(const struct timespec *__requested_time, struct timespec *__remaining);
#include <optional>

#include "base.cpp"
#include "grid.cpp"


struct ULP {

    bool multi_rule = false;

    uint64_t max_states = 0;

    struct {
        size_t var_count = 0;
        Variable** vars = nullptr;
        size_t func_count = 0;
        ClauseFunc** funcs = nullptr;
        size_t rule_func_index = 0;
        size_t clause_count = 0;
        Clause** clauses = nullptr;
    } graph;

    Grid grid = Grid(0, 0, 0);

    struct {

        bool periodic = false;
        uintmax_t periodic_dx = 0;
        uintmax_t periodic_dy = 0;
        uintmax_t periodic_period = 0;

        const char* after_rule_text = NULL;

        bool show_solutions = true;
        std::optional<uintmax_t> max_solutions = std::nullopt;

    } options;

    const Cell* get_rule_func() const {
        ClauseFunc* func = graph.funcs[graph.rule_func_index];
        if (func->vars != 10) {
            error("Rule function must have 10 inputs");
        }
        if (func->type != ClauseFuncType::LOOKUP_TABLE) {
            error("Rule function must be a lookup table");
        }
        return const_cast<const Cell*>(func->data.lookup_table.table);
    }

    void sync_search_grid() {
        for (size_t t = 0; t < grid.gens; t++) {
            for (size_t y = 0; y < grid.height; y++) {
                for (size_t x = 0; x < grid.width; x++) {
                    Cell value = grid.get(t, x, y);
                    if (c_is_var(value)) {
                        Variable* var = graph.vars[value];
                        if (var->value != V_UNKNOWN) {
                            grid.set(t, x, y, varvalue_to_cell(var->value));
                        }
                    }
                }
            }
        }
    }

};


struct {
    size_t set_unknown_cells;
    size_t start_unknown_cells;
    uintmax_t solutions_found;
    uintmax_t branches;
    double start_time;
} search_data;

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #ifdef _MSC_VER
        #include <intrin.h>
    #else
        #include <x86intrin.h>
    #endif
    static uint64_t read_counter() {
        return __rdtsc();
    }
#elif defined(__aarch64__)
    static uint64_t read_counter() {
        uint64_t val;
        __asm__ volatile("mrs %0, cntvct_el0" : "=r" (val));
        return val;
    }
#elif defined(__riscv) && __riscv_xlen == 64
    static uint64_t read_counter() {
        uint64_t cycles;
        __asm__ volatile("rdcycle %0" : "=r"(cycles));
        return cycles;
    }
#elif defined(__powerpc__) || defined(__ppc__)
    static uint64_t read_counter() {
        uint64_t tb;
        __asm__ volatile("mfspr %0, 268" : "=r"(tb));
        return tb;
    }
#else
    static uint64_t read_counter() {
        return __builtin_readcyclecounter();
    }
#endif


static double cycles_per_second;

static void calibrate_time() {
    struct timespec request;
    struct timespec remainder;
    // 0.01 seconds
    request.tv_sec = 0;
    request.tv_nsec = 10000000L;
    uint64_t start = read_counter();
    nanosleep(&request, &remainder);
    uint64_t end = read_counter();
    cycles_per_second = (end - start) * 100;
}

static double get_time() {
    return (double)(read_counter()) / cycles_per_second;
}

// cleans up the search state
void destroy_all();


namespace ulp_parser {

    static inline void check_ferror(FILE* file) {
        if (ferror(file)) {
            fclose(file);
            error("Could not read ULP (possibly corrupted)");
        }
    }

    int ulp_error_line = 1;

    static void _ulp_error(FILE* file, const char* format, ...) {
        long error_pos = ftell(file);
        if (error_pos == -1) {
            fclose(file);
            error("ftell() failed while parsing ULP: %s", strerror(errno));
        }
        if (fseek(file, 0, SEEK_SET) != 0) {
            fclose(file);
            error("fseek() failed while parsing ULP: %s", strerror(errno));
        }
        ulp_error_line = 1;
        while (true) {
            long current_pos = ftell(file);
            if (current_pos == -1) {
                fclose(file);
                error("ftell() failed while parsing ULP: %s", strerror(errno));
            } else if (current_pos >= error_pos) {
                break;
            }
            int c = fgetc(file);
            if (c == EOF) {
                break;
            } else if (c == '\n') {
                ulp_error_line++;
            }
        }
        fclose(file);
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        exit(EXIT_FAILURE);
    }

    #define ulp_error(file, format, ...) _ulp_error(file, "Invalid ULP (near line %i): " format, ulp_error_line __VA_OPT__(,) __VA_ARGS__)

    static inline int get_format_arg_count(const char* format) {
        int out = 0;
        for (size_t i = 0; format[i] != '\0'; i++) {
            if (format[i] == '%') {
                i++;
                char next = format[i];
                if (!(next == '%' || next == '*' || next == 'n')) {
                    out++;
                }
            }
        }
        return out;
    }

    // we need to add special code for check that adds in %n to determine
    // whether the correct number of characters was read for 0-argument format strings
    // we don't need to do this for check_line because that forces it to be a full line

    int check_count = 0;

    [[gnu::format(scanf, 2, 3)]] static bool _check(FILE* file, const char* format, ...) {
        long start_pos = ftell(file);
        if (start_pos == -1) {
            fclose(file);
            error("ftell() failed while parsing ULP: %s", strerror(errno));
        }
        // run vfscanf
        va_list args;
        va_start(args, format);
        int value = vfscanf(file, format, args);
        va_end(args);
        if (value == EOF) {
            check_ferror(file);
            if (fseek(file, start_pos, SEEK_SET) != 0) {
                fclose(file);
                error("fseek() failed while parsing ULP: %s", strerror(errno));
            }
            return false;
        }
        int count = get_format_arg_count(format);
        if (value == count) {
            if (count == 0) {
                if (check_count >= (int)strlen(format)) {
                    return true;
                } else {
                    return false;
                }
            } else {
                return true;
            }
        } else {
            if (fseek(file, start_pos, SEEK_SET) != 0) {
                fclose(file);
                error("fseek() failed while parsing ULP: %s", strerror(errno));
            }
            return false;
        }
    }

    #define check(file, format, ...) _check(file, format "%n" __VA_OPT__(,) __VA_ARGS__ , &check_count)

    [[gnu::format(scanf, 2, 3)]] static bool parse_line(FILE* file, const char* format, ...) {
        long start_pos = ftell(file);
        if (start_pos == -1) {
            fclose(file);
            error("Could not read ULP (ftell returned -1)");
        }
        // run vfscanf
        va_list args;
        va_start(args, format);
        int value = vfscanf(file, format, args);
        va_end(args);
        if (value == EOF) {
            check_ferror(file);
            if (fseek(file, start_pos, SEEK_SET) != 0) {
                fclose(file);
                error("fseek() failed while parsing ULP: %s", strerror(errno));
            }
            return false;
        }
        // printf("Testing '%s': %i out of %i\n", format, value, format_args);
        if (value == get_format_arg_count(format)) {
            // eat trailing whitespace on the line
            // it must find a newline or EOF!
            bool found = false;
            while (true) {
                int c = fgetc(file);
                if (c == EOF) {
                    check_ferror(file);
                    found = true;
                    break;
                } else if (c == '\n') {
                    ungetc(c, file);
                    found = true;
                    break;
                } else if (isspace(c)) {
                    continue;
                } else {
                    ungetc(c, file);
                    break;
                }
            }
            if (found) {
                return true;
            } else {
                if (fseek(file, start_pos, SEEK_SET) != 0) {
                    fclose(file);
                    error("fseek() failed while parsing ULP: %s", strerror(errno));
                }
                return false;
            }
        } else {
            if (fseek(file, start_pos, SEEK_SET) != 0) {
                fclose(file);
                error("fseek() failed while parsing ULP: %s", strerror(errno));
            }
            return false;
        }
    }

    // all these functions return true if no EOF, false if EOF

    static bool eat_whitespace(FILE* file) {
        while (true) {
            int c = fgetc(file);
            if (c == EOF) {
                check_ferror(file);
                // we've reached the end of the file
                return false;
            } else if (isspace(c)) {
                continue;
            } else {
                ungetc(c, file);
                break;
            }
        }
        return true;
    }

    static bool parse_graph_line(ULP& out, FILE* file) {
        if (!eat_whitespace(file)) {
            return false;
        }
        size_t v_size;
        if (parse_line(file, "var_count = %zu", &v_size)) {
            out.graph.var_count = v_size;
            // TODO: MAYBE THE SIZE IS ALREADY INCREMENTED BY 2???
            static_cast<const int>(1);
            size_t len = v_size + 2;
            // realloc behaves like malloc for null pointers
            out.graph.vars = (Variable**)safe_realloc(out.graph.vars, v_size * sizeof(Variable*));
        } else if (parse_line(file, "funcs : %zu", &v_size)) {
            if (out.graph.vars == nullptr) {
                ulp_error(file, "funcs provided before var_count");
            }
            out.graph.func_count = v_size;
            // realloc behaves like malloc for null pointers
            out.graph.funcs = (ClauseFunc**)safe_realloc(out.graph.funcs, v_size * sizeof(ClauseFunc*));
            for (size_t i = 0; i < v_size; i++) {
                size_t vars;
                if (!check(file, " %zu", &vars)) {
                    ulp_error(file, "No variable count provided for clause func %zu", i);
                }
                if (check(file, " lookup_table")) {
                    size_t len = 1UZ << vars;
                    ClauseFunc* func = (ClauseFunc*)safe_malloc(sizeof(ClauseFunc) + len * sizeof(Cell));
                    func->type = ClauseFuncType::LOOKUP_TABLE;
                    func->vars = vars;
                    for (size_t i = 0; i < len; i++) {
                        Cell value;
                        if (!check(file, " %zu", &value)) {
                            ulp_error(file, "Lookup table entry %zu missing for clause func %zu", value, i);
                        }
                        func->data.lookup_table.table[i] = value;
                    }
                } else {
                    ulp_error(file, "Invalid type for clause func %zu", i);
                }
            }
        } else {
            ulp_error(file, "Invalid line in [GRAPH]");
        }
    }

    static bool parse_grid_line(ULP& out, FILE* file) {
        if (!eat_whitespace(file)) {
            return false;
        }
    }

    static bool parse_options_line(ULP& out, FILE* file) {
        if (!eat_whitespace(file)) {
            return false;
        }
    }

    static void parse_file(ULP& out, FILE* file) {
        enum { NONE, GRAPH, GRID, OPTIONS } section = NONE;
        while (true) {
            if (!eat_whitespace(file)) {
                break;
            }
            size_t v_size;
            if (parse_line(file, "max_states = %zu", &v_size)) {
                out.max_states = v_size;
                section = NONE;
            } else if (parse_line(file, "[ GRAPH ]")) {
                section = GRAPH;
            } else if (parse_line(file, "[ GRID ]")) {
                section = GRID;
            } else if (parse_line(file, "[ OPTIONS ]")) {
                section = OPTIONS;
            } else if (section == GRAPH) {
                if (!parse_graph_line(out, file)) {
                    break;
                }
            } else if (section == GRID) {
                if (!parse_graph_line(out, file)) {
                    break;
                }
            } else if (section == OPTIONS) {
                if (!parse_graph_line(out, file)) {
                    break;
                }
            } else {
                ulp_error(file, "Invalid line\n");
            }
        }
    }

    #undef ulp_error
    #undef check

}
