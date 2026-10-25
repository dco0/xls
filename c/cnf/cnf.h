#pragma once

#include "../base/search.c"

typedef struct CNF_clauses {
    size_t clause_count;
    int lits[];
} CNF_clauses;

typedef struct CNF_vmap_entry {
    size_t variable;
    size_t num_states;
    size_t bits_per_var;
    CNF_vmap_entry *next;
    bool data[0];
} CNF_vmap_entry;

typedef struct CNF_vmap {
    size_t size;
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

static bool* cnf_vmap_entry_get(CNF_vmap_entry* entry, size_t i) {
    return entry->data + i * sizeof(*entry->data) * entry->bits_per_var;
}

CNF* cnf_encode(const ULP* ulp);
