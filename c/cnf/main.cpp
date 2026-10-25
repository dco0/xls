#include "internal.hpp"

namespace cnf {
    CNF_clauses* vec_to_clauses(const std::vector<std::vector<int>>& clauses) {
        size_t length = clauses.size();
        for (const auto& i : clauses) length += i.size();
        CNF_clauses* result = (CNF_clauses*) safe_malloc(sizeof(CNF_clauses) + sizeof(int) * length);
        result->clause_count = clauses.size();
        int* ptr = result->lits;
        for (const auto& i : clauses) {
            memcpy(ptr, i.data(), sizeof(int) * i.size());
            ptr += i.size();
            *ptr++ = 0;
        }
        return result;
    }

    CNF_vmap* vec_to_vmap(const std::vector<vmap_entry>& vmap) {
        size_t length = 0;
        for (const auto& i : vmap) {
            size_t num_states = i.x.size();
            size_t bits_per_var = i.x[0].size();
            size_t needed = sizeof(vmap_entry) + num_states * bits_per_var * sizeof(*CNF_vmap_entry::data);
            needed += -needed & alignof(CNF_vmap_entry)-1;
            length += needed;
        }
        CNF_vmap* result = (CNF_vmap*) safe_malloc(sizeof(CNF_vmap) + length);
        size_t offset = 0;
        size_t count = 0;
        for (const auto& i : vmap) {
            size_t num_states = i.x.size();
            size_t bits_per_var = i.x[0].size();
            size_t needed = sizeof(vmap_entry) + num_states * bits_per_var * sizeof(*CNF_vmap_entry::data);
            needed += -needed & alignof(CNF_vmap_entry)-1;
            CNF_vmap_entry& x = *(CNF_vmap_entry*)((char*)result->data + offset);
            offset += needed;
            x.variable = count++;
            x.num_states = num_states;
            x.bits_per_var = bits_per_var;
            x.next = offset == length ? NULL : (CNF_vmap_entry*)((char*)result->data + offset);
            bool* ptr = x.data;
            for (const auto& j : vmap) {
                memcpy(ptr, j.x.data(), j.x.size() * sizeof(*CNF_vmap_entry::data));
                ptr += j.x.size();
            }
        }
        return result;
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
    std::vector<std::vector<int>> clauses;
    std::vector<cnf::vmap_entry> vmap;

    // do stuff?!?!?!?

    cnf::preprocess_cnf(clauses);
    CNF* cnf = (CNF*) safe_malloc(sizeof(CNF));
    cnf->clauses = cnf::vec_to_clauses(clauses);
    cnf->vmap = cnf::vec_to_vmap(vmap);
    return cnf;
}
