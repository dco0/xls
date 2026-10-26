#include "internal.hpp"

namespace cnf {
    CNF_clauses* vec_to_clauses(const cnf_holder& holder) {
        size_t length = holder.clauses.size();
        for (const auto& i : holder.clauses) length += i.size();
        CNF_clauses* clauses = (CNF_clauses*) safe_malloc(sizeof(CNF_clauses) + sizeof(int) * length);
        clauses->clause_count = holder.clauses.size();
        clauses->var_count = holder.max_lit;
        int* ptr = clauses->lits;
        for (const auto& i : holder.clauses) {
            memcpy(ptr, i.data(), sizeof(int) * i.size());
            ptr += i.size();
            *ptr++ = 0;
        }
        return clauses;
    }

    CNF_vmap* vec_to_vmap(const cnf_holder& holder) {
        constexpr vmap_entry *_test = 0;
        constexpr size_t VMAP_OFFSET = (size_t)(_test+1);

        size_t stride = 0;
        for (const auto& i : holder.vmap) {
            size_t size = sizeof(CNF_vmap) + i.bits * i.values;
            size += size%VMAP_OFFSET ? VMAP_OFFSET-size%VMAP_OFFSET : 0;
            stride = size > stride ? size : stride;
        }
        CNF_vmap* vmap = (CNF_vmap*) safe_malloc(sizeof(CNF_vmap) + stride * holder.vmap.size());
        vmap->var_count = holder.vmap.size();
        vmap->stride = stride / VMAP_OFFSET;
        for (int i = 0; i < holder.vmap.size(); i++) {
            const vmap_entry& src = holder.vmap[i];
            vmap->data[stride * i] = {.bits = src.bits, .values = src.values};
            memcpy(vmap->data[stride * i].data, src.table.data(), src.bits * src.values);
        }
        return vmap;
    }

    struct _vec_empty { bool operator()(const std::vector<int>& x) { return x.size() == 0; } };
    void preprocess_cnf(std::vector<std::vector<int>>& cnf) {
        std::sort(cnf.begin(), cnf.end());
        if (cnf.size() == 0) return;
        if (cnf[0].size() == 0) {
            cnf.resize(1);
            return;
        }
        for (size_t i = 0; i < cnf.size()-1; i++) {
            if (cnf[i] == cnf[i+1]) cnf[i].clear();
        }
        cnf.erase(std::remove_if(cnf.begin(), cnf.end(), _vec_empty{}), cnf.end());
    }
} // namespace cnf

CNF* cnf_encode(const ULP* ulp) {
    cnf::cnf_holder clauses;

    // do stuff?!?!?!?

    cnf::preprocess_cnf(clauses.clauses);
    CNF* cnf = (CNF*) safe_malloc(sizeof(CNF));
    cnf->clauses = cnf::vec_to_clauses(clauses);
    cnf->vmap = cnf::vec_to_vmap(clauses);
    return cnf;
}
