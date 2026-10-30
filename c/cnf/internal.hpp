#pragma once

#include <vector>
#include <algorithm>

#include "cnf.hpp"

namespace cnf {
    typedef int lit;
    typedef std::vector<lit> clause;

    CNF_clauses* vec_to_clauses(const cnf_holder&);
    CNF_vmap* vec_to_vmap(const cnf_holder&);

    void preprocess_cnf(std::vector<std::vector<int>>&);

    struct cube { unsigned origin, span; };
    std::vector<cube> make_primes(size_t, const Cell*);
    void make_proposition(const std::vector<lit>&, const std::vector<cube>&, std::vector<std::vector<lit>>&);

    struct vmap_entry {
        size_t bits;
        size_t values;
        std::vector<char> table;
    };
    struct cnf_holder {
        size_t max_lit = 0;
        std::vector<clause> clauses;
        std::vector<vmap_entry> vmap;
        size_t lits(size_t count=1) {
            size_t res = max_lit+1;
            max_lit += count;
            return res;
        }
        cnf_holder& operator+=(const clause& clause) {
            clauses.push_back(clause);
            return *this;
        }
        cnf_holder& operator+=(const std::vector<clause>& clause) {
            clauses.insert(clauses.end(), clause.begin(), clause.end());
            return *this;
        }
    };

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
            } impl;
            ClauseFunc(::ULP*, ::ClauseFunc*);
            ~ClauseFunc();
            size_t vars() const;
            size_t bits_per_var() const;
            std::vector<bool> encode(Cell) const;
            std::vector<std::vector<lit>> enforce_constraint(const std::vector<std::vector<lit>>&) const;
        };
    }
}
