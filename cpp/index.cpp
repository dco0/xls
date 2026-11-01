
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "base/base.cpp"
#include "base/ulp.cpp"
#include "base/search.cpp"
#include "vls/implications.cpp"


int main(int argc, [[maybe_unused]] char** argv) {
    Search search;
    ULP& ulp = search.ulp;
    errno = 0;
    #if IN_BROWSER
        if (argc != 0) {
            error("Expected exactly 0 command line arguments\n");
        }
        static char* search_file = const_cast<char*>(SEARCH_FILE_GOES_HERE);
        FILE* file = fmemopen(search_file, sizeof(search_file) - 1, "r");
    #else
        if (argc != 1) {
            error("Expected exactly 1 command line argument\n");
        }
        FILE* file = fopen(argv[1], "r");
    #endif
    if (file == NULL) {
        error("Cannot read search file: %s", strerror(errno));
    }
    ULPParser parser = ULPParser(ulp, file);
    parser.parse();
    fclose(file);
    printf("Loaded search: %zu variables, %zu clauses\n", ulp.graph.var_count, ulp.graph.clause_count);
    vls::init_implications(ulp);
    vls::current_search = search;
}
