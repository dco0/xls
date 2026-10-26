#include "internal.hpp"

namespace cnf {
    struct entry { unsigned x, y, n; };

    std::vector<cube> make_primes(size_t n, const Cell* lut) {
        std::vector<entry> t, s;
        std::vector<cube> res;
        for (unsigned i = 0; i < 1<<n; i++) {
            if (!lut[i]) t.push_back({i, 0, 0});
            if (lut[i] >= 2) error("Multi-rule searching is not supported");
        }
        for (entry& c : t) for (const entry& d : t) {
            uint64_t diff = c.x ^ d.x;
            if (diff && !(diff & diff-1)) c.n |= diff;
        }
        for (unsigned u = 1; u; u <<= 1) {
            s.clear();
            std::vector<entry>::iterator i = t.begin(), j = t.begin();
            for (; i != t.end(); i++) {
                if (i->n) {
                    if (i->n >= 2*u) s.push_back(*i);
                    if (i->n & u && !(i->x & u)) {
                        while (j->y != i->y || j->x != (i->x | u)) j++;
                        s.push_back({i->x, i->y | u, i->n & j->n & ~u});
                    }
                }
                else res.push_back({i->x, i->y});
            }
            t.swap(s);
        }
        return res;
    }

    void make_proposition(const std::vector<lit>& vars, const std::vector<cube>& enc, cnf_holder& dest) {
        std::vector<int> clause;
        for (const cube& c : enc) {
            clause.clear();
            bool trivial = false;
            for (size_t i = 0; i < vars.size(); i++) {
                lit v = vars[i];
                if (c.origin>>i & 1) v = -v;
                if (!(c.span>>i & 1)) {
                    if (v == 1) trivial = true;
                    else if (v != -1) clause.push_back(v > 0 ? v-1 : v+1);
                }
            }
            if (!trivial) dest += clause;
        }
    }
}
