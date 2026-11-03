
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "base/base.cpp"
#include "base/ulp.cpp"
#include "base/search.cpp"
#include "vls/base.cpp"
#include "vls/implications.cpp"
#include "vls/search.cpp"
#include "vls/preprocess.cpp"


void run_search(Search& search) {
    Options& options = search.options;
    ULP& ulp = search.ulp;
    search.branches = 0;
    search.solutions_found = 0;
    search.set_unknown_vars = 0;
    search.start_unknown_vars = 0;
    for (size_t i = 0; i < ulp.var_count; i++) {
        if (!ulp.vars[i]->is_known()) {
            search.start_unknown_vars++;
        }
    }
    double time = get_time();
    search.start_time = time;
    search.last_progress_shown_time = time;
    search.last_max_partial_shown_time = time;
    if (options.backend == Options::Backend::VLS) {
        vls::run_search();
    } else {
        error("CNF searching is not supported yet");
    }
    search.print_max_partial_at_end();
}

int main(int argc, [[maybe_unused]] char** argv) {
    calibrate_time();
    // mark as static so destructor is called on exit()
    static Options options;
    static Search search = Search(options);
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
    printf("Loaded search: %zu variables, %zu clauses\n", ulp.var_count, ulp.clause_count);
    vls::init_implications(ulp);
    vls::search = &search;
    vls::preprocess();
    printf("Preprocessing complete: %zu variables, %zu clauses\n", ulp.var_count, ulp.clause_count);
    run_search(search);
}
