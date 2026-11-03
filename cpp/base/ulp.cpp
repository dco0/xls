
#pragma once

#include <stddef.h>
#include <errno.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <optional>
#include <map>

#include "base.cpp"
#include "grid.cpp"


struct Options {

    enum class Backend {CNF, VLS};
    Backend backend;

    size_t search_order_list_len = 0;
    // format: t, x, y
    size_t (*search_order_list)[3] = nullptr;

    bool periodic = false;
    uintmax_t periodic_dx = 0;
    uintmax_t periodic_dy = 0;
    uintmax_t periodic_period = 0;
    bool check_early_exhaustion = false;

    char* after_rule_text = nullptr;

    bool show_solutions = true;
    std::optional<uintmax_t> max_solutions = std::nullopt;

    double reporting_interval = 1.0;
    double max_partial_reporting_interval = 1.0;

    ~Options() {
        if (search_order_list != nullptr) {
            safe_free(search_order_list);
        }
        if (after_rule_text != nullptr) {
            safe_free(after_rule_text);
        }
    }

    bool show_max_partials = true;

};


class ULP {

public:

    bool multi_rule = false;

    uint64_t max_states = 0;

    size_t var_count = 0;
    Variable** vars = nullptr;

    size_t func_count = 0;
    ClauseFunc** funcs = nullptr;
    size_t rule_func_index = 0;

    size_t clause_count = 0;
    Clause** clauses = nullptr;

    Grid grid = Grid(0, 0, 0);

    Options& options;

    ULP(Options& options) : options(options) {
        // do nothing
    }

    ~ULP() {
        if (vars != nullptr) {
            for (size_t i = 0; i < var_count; i++) {
                safe_free(vars[i]);
            }
            safe_free(vars);
        }
        if (funcs != nullptr) {
            for (size_t i = 0; i < func_count; i++) {
                safe_free(funcs[i]);
            }
            safe_free(funcs);
        }
        if (clauses != nullptr) {
            for (size_t i = 0; i < clause_count; i++) {
                safe_free(clauses[i]);
            }
            safe_free(clauses);
        }
    }

    // UNFINISHED
    // // copy constructor
    // ULP(const ULP& other) : options(other.options) {
    //     // first the simple stuff
    //     multi_rule = other.multi_rule;
    //     max_states = other.max_states;
    //     var_count = other.var_count;
    //     func_count = other.func_count;
    //     funcs = safe_malloc<ClauseFunc*>(func_count);
    //     for (size_t i = 0; i < func_count; i++) {
    //         funcs[i] = other.funcs[i]->copy_allocating();
    //     }
    //     rule_func_index = other.rule_func_index;
    //     clause_count = other.clause_count;
    //     grid = other.grid;
    //     // now allocate them
    //     vars = safe_malloc<Variable*>(var_count);
    //     for (size_t i = 0; i < var_count; i++) {
    //         Variable* other_var = other.vars[i];
    //         Variable* var = safe_malloc<Variable>(1, sizeof(Variable) + other_var->use_count * sizeof(VarUse));
    //     }
    //     clauses = safe_malloc<Clause*>(clause_count);
    //     rule_func_index = other.rule_func_index;
    //     clause_count = other.clause_count;
    // }

    inline bool gc_is_var(GridCell cell) const {
        return cell >= max_states;
    }

    const GridCell* get_rule_func() const {
        ClauseFunc* func = funcs[rule_func_index];
        if (func->var_count != 10) {
            error("Rule function must have 10 inputs");
        }
        if (func->type != ClauseFunc::Type::LOOKUP_TABLE) {
            error("Rule function must be a lookup table");
        }
        return const_cast<const GridCell*>(func->data.lookup_table.table);
    }

    void sync_search_grid() {
        for (size_t t = 0; t < grid.gens; t++) {
            for (size_t y = 0; y < grid.height; y++) {
                for (size_t x = 0; x < grid.width; x++) {
                    GridCell value = grid.get(t, x, y);
                    if (gc_is_var(value)) {
                        Variable& var = *vars[value];
                        if (var.is_known()) {
                            grid.set(t, x, y, vv_to_gc(var.value));
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

    void check_ferror() {
        if (ferror(file)) {
            fclose(file);
            error("Could not read ULP (possibly corrupted)");
        }
    }

    long e_ftell() {
        long out = ftell(file);
        if (out == -1L) {
            fclose(file);
            error("ftell() failed while parsing ULP: %s", strerror(errno));
        }
        return out;
    }

    void e_fseek(long offset, int whence) {
        int failed = fseek(file, offset, whence);
        if (failed != 0) {
            fclose(file);
            error("fseek() failed while parsing ULP: %s", strerror(errno));
        }
    }

    int ulp_error_line = 1;

    void _ulp_error(const char* format, ...) {
        long error_pos = e_ftell();
        e_fseek(0, SEEK_SET);
        ulp_error_line = 1;
        while (true) {
            long current_pos = e_ftell();
            if (current_pos >= error_pos) {
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

    #define ulp_error(format, ...) _ulp_error("Invalid ULP (near line %i): " format "\n    at %s:%i\n" __VA_OPT__(,) __VA_ARGS__, __FILE__, __LINE__);

    int get_format_arg_count(const char* format) {
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
        long start_pos = e_ftell();
        // run vfscanf
        va_list args;
        va_start(args, format);
        int value = vfscanf(file, format, args);
        va_end(args);
        if (value == EOF) {
            check_ferror();
            e_fseek(start_pos, SEEK_SET);
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
            e_fseek(start_pos, SEEK_SET);
            return false;
        }
    }

    #define check(format, ...) _check(format "%n" __VA_OPT__(,) __VA_ARGS__ , &check_count)

    [[gnu::format(scanf, 2, 3)]] bool check_line(const char* format, ...) {
        long start_pos = e_ftell();
        // run vfscanf
        va_list args;
        va_start(args, format);
        int value = vfscanf(file, format, args);
        va_end(args);
        if (value == EOF) {
            check_ferror();
            e_fseek(start_pos, SEEK_SET);
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
                e_fseek(start_pos, SEEK_SET);
                return false;
            }
        } else {
            e_fseek(start_pos, SEEK_SET);
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

    void parse_graph_line() {
        size_t v_size;
        if (check_line("var_count = %zu", &v_size)) {
            out.var_count = v_size;
            // realloc behaves like malloc for null pointers
            out.vars = safe_realloc(out.vars, v_size);
            // fill in the implicit vars
            for (size_t i = 0; i < out.max_states; i++) {
                Variable* var = safe_malloc<Variable>(1, sizeof(Variable));
                out.vars[i] = var;
                var->number = i;
            }
        } else if (check_line("funcs : %zu", &v_size)) {
            if (out.vars == nullptr) {
                ulp_error("funcs provided before var_count");
            }
            out.func_count = v_size;
            // realloc behaves like malloc for null pointers
            out.funcs = safe_realloc<ClauseFunc*>(out.funcs, v_size);
            for (size_t i = 0; i < v_size; i++) {
                size_t var_count;
                if (!check(" %zu", &var_count)) {
                    ulp_error("No variable count provided for clause func %zu", i);
                }
                if (check(" lookup_table")) {
                    size_t len = 1UZ << var_count;
                    ClauseFunc* func = safe_malloc<ClauseFunc>(1, sizeof(ClauseFunc) + len * sizeof(GridCell));
                    func->type = ClauseFunc::Type::LOOKUP_TABLE;
                    func->var_count = var_count;
                    for (size_t i = 0; i < len; i++) {
                        GridCell value;
                        if (!check(" %zu", &value)) {
                            ulp_error("Lookup table entry %zu for clause func %zu is missing or invalid", value, i);
                        }
                        func->data.lookup_table.table[i] = value;
                    }
                } else {
                    ulp_error("Invalid type for clause func %zu", i);
                }
                if (!check_line("")) {
                    ulp_error("Extra non-whitespace characters at end of clause func");
                }
            }
        } else if (check_line("rule_func_index = %zu", &v_size)) {
            out.rule_func_index = v_size;
        } else if (check_line("clauses : %zu", &v_size)) {
            if (out.vars == nullptr) {
                ulp_error("clauses provided before var_count");
            }
            // determine variable use counts first
            long start = e_ftell();
            std::map<size_t, size_t> use_counts;
            for (size_t i = 0; i < v_size; i++) {
                size_t func_number;
                if (!check(" %zu", &func_number)) {
                    ulp_error("No func number provided for clause %zu", i);
                }
                size_t vars;
                if (!check(" %zu", &vars)) {
                    ulp_error("No variable count provided for clause %zu", i);
                }
                for (size_t j = 0; j < vars; j++) {
                    size_t value;
                    if (!check(" %zu", &value)) {
                        ulp_error("Variable %zu for clause %zu is missing or invalid", j, i);
                    }
                    // fine because it will initialize it to 0 if it doesn't exist
                    use_counts[value]++;
                }
                if (!check_line("")) {
                    ulp_error("Extra non-whitespace characters at end of clause func");
                }
            }
            e_fseek(start, SEEK_SET);
            // now allocate the variables
            for (size_t i = out.max_states; i < out.var_count; i++) {
                Variable* var = safe_malloc<Variable>(1, sizeof(Variable) + use_counts[i] * sizeof(VarUse));
                var->number = i;
                var->value = VV_UNKNOWN;
                // set it to 0 so the next part can increment it to track how many have been assigned
                var->use_count = 0;
                out.vars[i] = var;
            }
            // finally, do the clauses
            out.clause_count = v_size;
            // realloc behaves like malloc for null pointers
            out.clauses = safe_realloc(out.clauses, v_size);
            for (size_t i = 0; i < out.clause_count; i++) {
                size_t func_number;
                if (!check(" %zu", &func_number)) {
                    ulp_error("No func number provided for clause %zu", i);
                }
                if (func_number >= out.func_count) {
                    ulp_error("Invalid func number provided for clause %zu: %zu", i, func_number);
                }
                ClauseFunc* func = out.funcs[func_number];
                size_t var_count;
                if (!check(" %zu", &var_count)) {
                    ulp_error("No variable count provided for clause %zu", i);
                }
                Clause* clause = safe_malloc<Clause>(1, sizeof(Clause) + var_count * sizeof(Variable*));
                clause->number = i;
                clause->func = func;
                clause->implication_pointer = nullptr;
                clause->var_count = var_count;
                for (size_t j = 0; j < var_count; j++) {
                    size_t var_number;
                    if (!check(" %zu", &var_number)) {
                        ulp_error("Variable %zu for clause %zu is missing or invalid", j, i);
                    }
                    Variable* var = out.vars[var_number];
                    clause->vars[i] = var;
                    VarUse& use = var->uses[var->use_count];
                    use.clause = clause;
                    use.change = POWERS_OF_3[j];
                    var->use_count++;
                }
                if (!check_line("")) {
                    ulp_error("Extra non-whitespace characters at end of clause func");
                }
            }
        } else {
            ulp_error("Invalid line in [GRAPH]");
        }
    }

    size_t grid_width = 0;
    size_t grid_height = 0;
    size_t grid_gens = 0;

    void parse_grid_line() {
        size_t v_size;
        if (check_line("width = %zu", &v_size)) {
            grid_width = v_size;
        } else if (check_line("height = %zu", &v_size)) {
            grid_height = v_size;
        } else if (check_line("gens = %zu", &v_size)) {
            grid_gens = v_size;
        } else if (check_line("data : %zu", &v_size)) {
            if (grid_width == 0 || grid_height == 0 || grid_gens == 0) {
                ulp_error("Not all of width, height, and gens are provided before grid data (or are zero)");
            }
            out.grid.~Grid();
            new (&out.grid) Grid(grid_width, grid_height, grid_gens);
            if (out.grid.total_size != v_size) {
                ulp_error("Grid size vs grid data list size mismatch (true size is %zu, given size is %zu)", out.grid.total_size, v_size);
            }
            for (size_t i = 0; i < out.grid.total_size; i++) {
                GridCell value;
                if (!check(" %zu", &value)) {
                    ulp_error("Grid item at index %zu is missing or invalid", i);
                }
                out.grid.data[i] = value;
            }
        } else {
            ulp_error("Invalid line in [GRID]");
        }
    }

    void parse_options_line() {
        Options& options = out.options;
        int v_int;
        double v_double;
        size_t v_size;
        size_t v_size_2;
        size_t v_size_3;
        uintmax_t v_uintmax;
        char v_str[256];
        if (check_line("backend = %255s", v_str)) {
            if (strcmp(v_str, "cnf") == 0) {
                options.backend = Options::Backend::CNF;
            } else if (strcmp(v_str, "vls") == 0) {
                options.backend = Options::Backend::VLS;
            } else {
                ulp_error("Invalid backend: '%s'", v_str);
            }
        } else if (check_line("search_order_list : %zu", &v_size)) {
            size_t len = v_size;
            options.search_order_list_len = len;
            // realloc behaves like malloc for null pointers
            options.search_order_list = safe_realloc(options.search_order_list, len);
            for (size_t i = 0; i < len; i++) {
                if (!check_line("%zu %zu %zu", &v_size, &v_size_2, &v_size_3)) {
                    ulp_error("Search order list item %zu is missing or invalid", i);
                }
                size_t* item = options.search_order_list[i];
                item[0] = v_size;
                item[1] = v_size_2;
                item[2] = v_size_3;
            }
        } else if (check_line("periodic = ( %zu , %zu ) c / %zu", &v_size, &v_size_2, &v_size_3)) {
            options.periodic = true;
            options.periodic_dx = v_size;
            options.periodic_dy = v_size_2;
            options.periodic_period = v_size_3;
        } else if (check_line("after_rule_text = %255s", v_str)) {
            size_t len = strnlen(v_str, 255);
            options.after_rule_text = safe_malloc<char>(len + 1);
            strncpy(options.after_rule_text, v_str, len);
        } else if (check_line("show_solutions = %i", &v_int)) {
            options.show_solutions = v_int;
        } else if (check_line("max_solutions = %zu", &v_uintmax)) {
            options.max_solutions = v_uintmax;
        } else if (check_line("show_max_partials = %i", &v_int)) {
            options.show_max_partials = v_int;
        } else if (check_line("reporting_interval = %lf", &v_double)) {
            options.reporting_interval = v_double;
        } else if (check_line("max_partial_reporting_interval = %lf", &v_double)) {
            options.max_partial_reporting_interval = v_double;
        } else {
            ulp_error("Invalid line in [OPTIONS]");
        }
    }

public:    

    void parse() {
        enum {NONE, GRAPH, GRID, OPTIONS} section = NONE;
        while (true) {
            if (!eat_whitespace()) {
                break;
            }
            size_t v_size;
            if (check_line("max_states = %zu", &v_size)) {
                out.max_states = v_size;
                section = NONE;
            } else if (check_line("[ GRAPH ]")) {
                section = GRAPH;
            } else if (check_line("[ GRID ]")) {
                section = GRID;
            } else if (check_line("[ OPTIONS ]")) {
                section = OPTIONS;
            } else if (section == GRAPH) {
                parse_graph_line();
            } else if (section == GRID) {
                parse_grid_line();
            } else if (section == OPTIONS) {
                parse_options_line();
            } else {
                ulp_error("Invalid line\n");
            }
        }
    }

    #undef ulp_error
    #undef check

};
