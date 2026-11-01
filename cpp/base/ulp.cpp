
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


class ULPParser {

private:

    ULP& out;
    FILE* file;

public:

    ULPParser(ULP& ulp, FILE* file) : out(ulp) {
        this->file = file;
    }

private:

    inline void check_ferror() {
        if (ferror(file)) {
            fclose(file);
            error("Could not read ULP (possibly corrupted)");
        }
    }

    int ulp_error_line = 1;

    void _ulp_error(const char* format, ...) {
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

    #define ulp_error(format, ...) _ulp_error("Invalid ULP (near line %i): " format, ulp_error_line __VA_OPT__(,) __VA_ARGS__)

    inline int get_format_arg_count(const char* format) {
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

    [[gnu::format(scanf, 2, 3)]] bool _check(const char* format, ...) {
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
            check_ferror();
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

    #define check(format, ...) _check(format "%n" __VA_OPT__(,) __VA_ARGS__ , &check_count)

    [[gnu::format(scanf, 2, 3)]] bool parse_line(const char* format, ...) {
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
            check_ferror();
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
                    check_ferror();
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

    bool eat_whitespace() {
        while (true) {
            int c = fgetc(file);
            if (c == EOF) {
                check_ferror();
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

    bool parse_graph_line() {
        if (!eat_whitespace()) {
            return false;
        }
        size_t v_size;
        if (parse_line("var_count = %zu", &v_size)) {
            out.graph.var_count = v_size;
            // TODO: MAYBE THE SIZE IS ALREADY INCREMENTED BY 2???
            static_cast<const int>(1);
            size_t len = v_size + 2;
            // realloc behaves like malloc for null pointers
            out.graph.vars = (Variable**)safe_realloc(out.graph.vars, v_size * sizeof(Variable*));
        } else if (parse_line("funcs : %zu", &v_size)) {
            if (out.graph.vars == nullptr) {
                ulp_error("funcs provided before var_count");
            }
            out.graph.func_count = v_size;
            // realloc behaves like malloc for null pointers
            out.graph.funcs = (ClauseFunc**)safe_realloc(out.graph.funcs, v_size * sizeof(ClauseFunc*));
            for (size_t i = 0; i < v_size; i++) {
                size_t vars;
                if (!check(" %zu", &vars)) {
                    ulp_error("No variable count provided for clause func %zu", i);
                }
                if (check(" lookup_table")) {
                    size_t len = 1UZ << vars;
                    ClauseFunc* func = (ClauseFunc*)safe_malloc(sizeof(ClauseFunc) + len * sizeof(Cell));
                    func->type = ClauseFuncType::LOOKUP_TABLE;
                    func->vars = vars;
                    for (size_t i = 0; i < len; i++) {
                        Cell value;
                        if (!check(" %zu", &value)) {
                            ulp_error("Lookup table entry %zu missing for clause func %zu", value, i);
                        }
                        func->data.lookup_table.table[i] = value;
                    }
                } else {
                    ulp_error("Invalid type for clause func %zu", i);
                }
            }
        } else {
            ulp_error("Invalid line in [GRAPH]");
        }
    }

    bool parse_grid_line() {
        if (!eat_whitespace()) {
            return false;
        }
    }

    bool parse_options_line() {
        if (!eat_whitespace()) {
            return false;
        }
    }

public:    

    void parse() {
        enum { NONE, GRAPH, GRID, OPTIONS } section = NONE;
        while (true) {
            if (!eat_whitespace()) {
                break;
            }
            size_t v_size;
            if (parse_line("max_states = %zu", &v_size)) {
                out.max_states = v_size;
                section = NONE;
            } else if (parse_line("[ GRAPH ]")) {
                section = GRAPH;
            } else if (parse_line("[ GRID ]")) {
                section = GRID;
            } else if (parse_line("[ OPTIONS ]")) {
                section = OPTIONS;
            } else if (section == GRAPH) {
                if (!parse_graph_line()) {
                    break;
                }
            } else if (section == GRID) {
                if (!parse_graph_line()) {
                    break;
                }
            } else if (section == OPTIONS) {
                if (!parse_graph_line()) {
                    break;
                }
            } else {
                ulp_error("Invalid line\n");
            }
        }
    }

    #undef ulp_error
    #undef check

};
