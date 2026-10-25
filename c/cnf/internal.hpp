#pragma once

#include <vector>
#include <algorithm>

extern "C" {
    #include "cnf.h"
}

namespace cnf {
    typedef int lit;

    struct vmap_entry {
        std::vector<std::vector<bool>> x;
    };
    CNF_clauses* vec_to_clauses(const std::vector<std::vector<lit>>&);
    CNF_vmap* vec_to_vmap(const std::vector<vmap_entry>&);

    void preprocess_cnf(std::vector<std::vector<int>>&);

    struct cube { unsigned origin, span; };
    std::vector<cube> make_primes(size_t, const Cell*);
    void make_proposition(const std::vector<lit>&, const std::vector<cube>&, std::vector<std::vector<lit>>&);

    namespace cf {
        enum class Type {
            BINARY_LUT, LOOKUP_TABLE
        };
        struct BinaryLut;
        struct LookupTable;
        struct ClauseFunc {
            const ::ClauseFunc* cf;
            Type type;
            struct {
                const BinaryLut* binary_lut;
                const LookupTable* lookup_table;
            } impl;
            ClauseFunc(::ULP* ULP, ::ClauseFunc*);
            ~ClauseFunc();
            size_t vars() const;
            size_t bits_per_var() const;
            std::vector<bool> encode(Cell) const;
            Cell decode(const std::vector<bool>&) const;
            std::vector<std::vector<lit>> enforce_constraint(const std::vector<std::vector<lit>>&) const;
        };
    }
}
