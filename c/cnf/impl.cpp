#include "internal.hpp"

#define unreachable __builtin_unreachable();

namespace cnf::cf {
    struct BinaryLut {
        const ::LookupTableClauseFunc* cf;
        const ::ULP* ulp;
        mutable bool cached = false;
        mutable std::vector<cube> cache;
        BinaryLut(::ULP* ulp, ::LookupTableClauseFunc* cf) : ulp(ulp), cf(cf) {}
        size_t vars() const {
            return cf->vars;
        }
        size_t bits_per_var() const {
            return 2;
        }
        std::vector<bool> encode(Cell cell) const {
            return {(bool)cell};
        }
        std::vector<std::vector<lit>> enforce_constraint(const std::vector<std::vector<lit>>& cells) const {
            if (!cached) {
                cache = make_primes(cf->vars, cf->data);
                cached = true;
            }
            std::vector<lit> flatcells(cells.size());
            for (size_t i = 0; i < cells.size(); i++) flatcells[i] = cells[i][0];
            std::vector<std::vector<lit>> clauses;
            make_proposition(flatcells, cache, clauses);
            return clauses;
        }
    };
}

namespace cnf::cf {
    ClauseFunc::ClauseFunc(::ULP* ulp, ::ClauseFunc* cf) : cf(cf) {
        switch (cf->type) {
            case (::ClauseFuncType::CFT_LOOKUP_TABLE):
                if (ulp->max_states != 2) error("not implemented");
                impl.binary_lut = new BinaryLut{ulp, &cf->data.lookup_table};
                type = Type::BINARY_LUT;
                break;
            default:
                error("Invalid clause func");
        }
    }
    ClauseFunc::~ClauseFunc() {
        switch (type) {
            case (Type::BINARY_LUT):
                delete impl.binary_lut;
        }
    }
    size_t ClauseFunc::vars() const {
        switch (type) {
            case (Type::BINARY_LUT):
                return impl.binary_lut->vars();
        }
        unreachable
    }
    size_t ClauseFunc::bits_per_var() const {
        switch (type) {
            case (Type::BINARY_LUT):
                return impl.binary_lut->bits_per_var();
        }
        unreachable
    }
    std::vector<bool> ClauseFunc::encode(Cell cell) const {
        switch (type) {
            case (Type::BINARY_LUT):
                return impl.binary_lut->encode(cell);
        }
        unreachable
    }
    std::vector<std::vector<int>> ClauseFunc::enforce_constraint(const std::vector<std::vector<int>>& cells) const {
        switch (type) {
            case (Type::BINARY_LUT):
                return impl.binary_lut->enforce_constraint(cells);
        }
        unreachable
    }
}
