#pragma once

#include "../base/search.c"

typedef struct CNF_clauses {
    size_t var_count;
    size_t clause_count;
    int lits[];
} CNF_clauses;

typedef struct CNF_vmap_entry {
    size_t bits;
    size_t values;
    bool data[0];
} CNF_vmap_entry;

typedef struct CNF_vmap {
    size_t var_count;
    size_t stride;
    CNF_vmap_entry data[];
} CNF_vmap;

typedef struct CNF {
    CNF_clauses* clauses;
    CNF_vmap* vmap;
} CNF;

static void cnf_free(CNF* cnf) {
    safe_free(cnf->clauses);
    safe_free(cnf->vmap);
    safe_free(cnf);
}

CNF* cnf_encode(const ULP* ulp);
