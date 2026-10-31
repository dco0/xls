
#pragma once

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

    const Variable** get_rule_func() {
        ClauseFunc* func = graph.funcs[graph.rule_func_index];
        if (func->vars != 10) {
            error("Rule function must have 10 inputs");
        }
        if (func->type != ClauseFuncType::LOOKUP_TABLE) {
            error("Rule function must be a lookup table");
        }
        return const_cast<const Variable**>(func->data.lookup_table.table);
    }

    void get_rule_func_cells(Cell* out) {
        const Variable** func = get_rule_func();
        for (size_t i = 0; i < 1024; i++) {
            out[i] = func[i]->value;
        }
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

static ULP s;


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

    #define ulp_error(format, ...) \
        do { \
            long pos = ftell(file); \
            fclose(file); \
            error("Invalid ULP (near position %li): " format, pos __VA_OPT__(,) __VA_ARGS__); \
        } while (0);

    [[gnu::format(scanf, 2, 3)]] static bool parse_line(FILE* file, const char* format, ...) {
        // run vfscanf
        long start_pos = ftell(file);
        if (start_pos == -1) {
            fclose(file);
            error("Could not read ULP (ftell returned -1)");
        }
        va_list args;
        va_start(args, format);
        int value = vfscanf(file, format, args);
        va_end(args);
        if (value == EOF) {
            check_ferror(file);
            fseek(file, start_pos, SEEK_SET);
            return false;
        }
        // get number of format args
        int format_args = 0;
        for (size_t i = 0; format[i] != '\0'; i++) {
            if (format[i] == '%') {
                char next = format[i + 1];
                if (next == '%' || next == '*' || next == 'n') {
                    i++;
                    continue;
                } else {
                    format_args++;
                }
            }
        }
        // printf("Testing '%s': %i out of %i\n", format, value, format_args);
        if (value == format_args) {
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
                fseek(file, start_pos, SEEK_SET);
                return false;
            }
        } else {
            fseek(file, start_pos, SEEK_SET);
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
            size_t size = out.graph.var_count * sizeof(Variable*);
            if (out.graph.vars != nullptr) {
                out.graph.vars = (Variable**)safe_realloc(out.graph.vars, size);
            } else {
                out.graph.vars = (Variable**)safe_malloc(size);
            }
        } else {
            ulp_error("Invalid line in [GRAPH]");
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

    static void parse_ulp(ULP& out, FILE* file) {
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
                ulp_error("Invalid line");
            }
        }
    }

    static inline bool parse_part_of_file(ULP& out, FILE* file) {

        int v_int;
        double v_double;
        size_t v_size;
        intmax_t v_intmax;
        intmax_t v_intmax_2;
        uintmax_t v_uintmax;
        char v_str[256];
        if (check_line(file, "width = %zu", &v_size)) {
            data->width = v_size;
        } else if (check_line(file, "height = %zu", &v_size)) {
            data->height = v_size;
        } else if (check_line(file, "gens = %zu", &v_size)) {
            data->gens = v_size;
        } else if (check_line(file, "var_count = %zu", &v_size)) {
            data->var_count = v_size;
        } else if (check_line(file, "grid:")) {
            size_t total_size = data->width * data->height * data->gens;
            data->grid = safe_malloc(total_size * sizeof(InitFromCell));
            for (size_t i = 0; i < total_size; i++) {
                InitFromCell cell;
                cell.var = 0;
                cell.no_clause = false;
                size_t value;
                if (fscanf(file, " %zu", &value) != 1) {
                    check_ferror(file);
                    search_file_error("Missing grid element");
                }
                if (value == 0) {
                    cell.value = OFF;
                } else if (value == 1) {
                    cell.value = ON;
                } else {
                    cell.value = UNKNOWN;
                    value -= 1;
                    cell.var = value;
                }
                // check apostrophe
                int c = fgetc(file);
                check_ferror(file);
                if (c == EOF) {
                    continue;
                } else if (c == '\'') {
                    cell.no_clause = true;
                } else {
                    ungetc(c, file);
                }
                memcpy(&(data->grid[i]), &cell, sizeof(InitFromCell));
            }
        } else if (check_line(file, "multi_rule = %i", &v_int)) {
            state.multi_rule = v_int;
        } else if (check_line(file, "rule_func:")) {
            for (size_t i = 0; i < 1024; i++) {
                int value;
                if (fscanf(file, " %i", &value) != 1) {
                    check_ferror(file);
                    search_file_error("Missing rule function value");
                }
                state.rule_func[i] = value;
            }
        } else if (check_line(file, "search_order ( length = %zu ):", &v_size)) {
            config.search_order_len = v_size;
            config.search_order = safe_malloc(config.search_order_len * sizeof(size_t[3]));
            for (size_t i = 0; i < config.search_order_len; i++) {
                size_t t;
                size_t x;
                size_t y;
                if (fscanf(file, " %zu %zu %zu", &t, &x, &y) != 3) {
                    check_ferror(file);
                    search_file_error("Missing search order item");
                }
                config.search_order[i][0] = t;
                config.search_order[i][1] = x;
                config.search_order[i][2] = y;
            }
        } else if (check_line(file, "initial_value = %i", &v_int)) {
            config.initial_value = v_int == 0 ? OFF : ON;
        } else if (check_line(file, "periodic = ( %ji , %ji ) c / %zu", &v_intmax, &v_intmax_2, &v_size)) {
            config.periodic = true;
            config.periodic_dx = v_intmax;
            config.periodic_dy = v_intmax_2;
            config.periodic_period = v_size;
        } else if (check_line(file, "check_early_exhaustion = %i", &v_int)) {
            config.check_early_exhaustion = v_int;
        } else if (check_line(file, "top_padding = %zu", &v_size)) {
            config.top_padding = v_size;
        } else if (check_line(file, "bottom_padding = %zu", &v_size)) {
            config.bottom_padding = v_size;
        } else if (check_line(file, "left_padding = %zu", &v_size)) {
            config.left_padding = v_size;
        } else if (check_line(file, "right_padding = %zu", &v_size)) {
            config.right_padding = v_size;
        } else if (check_line(file, "start_padding = %zu", &v_size)) {
            config.start_padding = v_size;
        } else if (check_line(file, "end_padding = %zu", &v_size)) {
            config.end_padding = v_size;
        } else if (check_line(file, "rule_format_string = %255s", v_str)) {
            size_t len = strnlen(v_str, 255);
            config.rule_format_string = safe_malloc((len + 1) * sizeof(char));
            memset(config.rule_format_string, '\0', len + 1);
            strncpy(config.rule_format_string, v_str, len);
        } else if (check_line(file, "reporting_interval = %lf", &v_double)) {
            config.reporting_interval = v_double;
        } else if (check_line(file, "show_solutions = %i", &v_int)) {
            config.show_solutions = v_int;
        } else if (check_line(file, "max_solutions = %ju", &v_uintmax)) {
            config.max_solutions = v_uintmax;
        } else if (check_line(file, "filter_empty_solutions = %i", &v_int)) {
            config.filter_empty_solutions = v_int;
        } else if (check_line(file, "filter_duplicate_solutions = %i", &v_int)) {
            config.filter_duplicate_solutions = v_int;
        } else if (check_line(file, "filter_subperiod_solutions = %i", &v_int)) {
            config.filter_subperiod_solutions = v_int;
        } else if (check_line(file, "cell_period_filter ( length = %zu ) :", &v_size)) {
            config.cell_period_filter_length = v_size;
            config.cell_period_filter = safe_malloc(config.cell_period_filter_length * sizeof(int));
            for (size_t i = 0; i < config.cell_period_filter_length; i++) {
                size_t value;
                if (fscanf(file, " %zu", &value) != 1) {
                    check_ferror(file);
                    search_file_error("Missing cell period filter item");
                }
                config.cell_period_filter[i] = value;
            }
        } else if (check_line(file, "max_partials = %i", &v_int)) {
            config.max_partials = v_int;
        } else if (check_line(file, "max_partial_scoring = %i", &v_int)) {
            config.max_partial_scoring = v_int;
        } else if (check_line(file, "max_partial_reporting_interval = %lf", &v_double)) {
            config.max_partial_reporting_interval = v_double;
        } else {
            if (feof(file)) {
                return false;
            } else {
                search_file_error("Unrecognized line");
            }
        }
        return true;
    }

    #undef ulp_error

    #if IN_BROWSER
    char search_file[] = SEARCH_FILE_GOES_HERE;
    #endif
    static inline void init_state_and_config_from_file([[maybe_unused]] const char* path) {
        errno = 0;
        #if IN_BROWSER
            FILE* file = fmemopen(search_file, sizeof(search_file) - 1, "r");
        #else
            FILE* file = fopen(path, "r");
        #endif
        if (file == NULL) {
            error("Cannot read search file: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        InitFromState data = {
            .height = 0,
            .width = 0,
            .gens = 0,
            .var_count = 0,
            .grid = NULL,
        };
        while (true) {
            if (!parse_part_of_file(file, &data)) {
                break;
            }
        }
        fclose(file);
        // call this here rather than in the main function
        // so that the clause manipulation in init_state works fine
        init_implications();
        init_state(&data);
        if (data.grid != NULL) {
            safe_free(data.grid);
        }
        printf("Loaded search: %zu variables, %zu clauses\n", state.var_count, state.clause_count);
    }

}
