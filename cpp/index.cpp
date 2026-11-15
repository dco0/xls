
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <cstdlib>

#include "base/params.cpp" // IWYU pragma: keep
#include "base/base.cpp"
#include "base/grid.cpp"
#include "base/ulp.cpp"
#include "base/search.cpp"
#include "vls/base.cpp"
#include "vls/implications.cpp"
#include "vls/search.cpp"
#include "vls/preprocess.cpp"


void run_search(Search& search) {
    ULP& ulp = search.ulp;
    Graph& graph = *ulp.graph;
    Options& options = search.options;
    search.branches = 0;
    search.solutions_found = 0;
    search.set_unknown_vars = 0;
    search.start_unknown_vars = 0;
    for (size_t i = 0; i < graph.var_count; i++) {
        if (!graph.vars[i]->is_known()) {
            search.start_unknown_vars++;
        }
    }
    double time = get_time();
    search.start_time = time;
    search.last_progress_shown_time = time;
    search.last_max_partial_shown_time = time;
    if (options.backend == Options::Backend::VLS) {
        vls::run_search();
    } else if (options.backend == Options::Backend::CNF) {
        error("CNF searching is not supported yet");
    } else {
        error("No backend provided");
    }
    search.print_max_partial_at_end();
}

int main(int argc, [[maybe_unused]] char** argv) {
    calibrate_time();
    std::atexit(Grid::destroy);
    // mark as static so destructor is called on exit()
    static Graph graph;
    static Search search = Search(&graph);
    ULP& ulp = search.ulp;
    errno = 0;
    #if IN_BROWSER
        if (argc != 1) {
            error("Expected exactly 0 command line arguments");
        }
        static char* search_file = const_cast<char*>(SEARCH_FILE_GOES_HERE);
        FILE* file = fmemopen(search_file, sizeof(search_file) - 1, "r");
    #else
        if (argc != 2) {
            error("Expected exactly 1 command line argument");
        }
        FILE* file = fopen(argv[1], "r");
    #endif
    if (file == NULL) {
        error("Cannot read search file: %s", strerror(errno));
    }
    ULPParser parser = ULPParser(ulp, file);
    parser.parse();
    fclose(file);
    printf("Loaded search: %zu variables, %zu clauses\n", graph.var_count - graph.max_states, graph.clause_count);
    debug(1, search.print_grid(stdout));
    vls::load_search(search);
    vls::init_implications();
    vls::preprocess();
    printf("Preprocessing complete: %zu variables, %zu clauses\n", graph.var_count - graph.max_states, graph.clause_count);
    debug(1, search.print_grid(stdout));
    run_search(search);
}
