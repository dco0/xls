
#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "../base/params.cpp"
#include "../base/base.cpp"
#include "../base/ulp.cpp"
#include "../base/rule.cpp"
#include "../base/solutions.cpp"
#include "stack.cpp"
#include "implications.cpp"


namespace vls {


class PreClause {

public:

    Clause& real_clause;
    Variable* nw;
    Variable* n;
    Variable* ne;
    Variable* w;
    Variable* c;
    Variable* e;
    Variable* sw;
    Variable* s;
    Variable* se;
    Variable* next;

    PreClause(Clause& clause) : real_clause(clause) {
        if (clause.var_count != 10) {
            error("Creation of PreClause with clause whose variable count is not 10 (it is %zu)", clause.var_count);
        }
        next = clause.vars[0];
        se = clause.vars[1];
        e = clause.vars[2];
        ne = clause.vars[3];
        s = clause.vars[4];
        c = clause.vars[5];
        n = clause.vars[6];
        sw = clause.vars[7];
        w = clause.vars[8];
        nw = clause.vars[9];
    }

    PreClause(Clause& clause, Variable* nw, Variable* n, Variable* ne, Variable* w, Variable* c, Variable* e, Variable* sw, Variable* s, Variable* se, Variable* next) : real_clause(clause) {
        this->nw = nw;
        this->n = n;
        this->ne = ne;
        this->w = w;
        this->c = c;
        this->e = e;
        this->sw = sw;
        this->s = s;
        this->se = se;
        this->next = next;
    }

    PreClause(const PreClause& clause) : real_clause(clause.real_clause) {
        this->nw = clause.nw;
        this->n = clause.n;
        this->ne = clause.ne;
        this->w = clause.w;
        this->c = clause.c;
        this->e = clause.e;
        this->sw = clause.sw;
        this->s = clause.s;
        this->se = clause.se;
        this->next = clause.next;
    }

    size_t get_number() {
        return real_clause.number;
    }

    bool operator==(const PreClause& other) const {
        return nw == other.nw
            && n == other.n
            && ne == other.ne
            && w == other.w
            && c == other.c
            && e == other.e
            && sw == other.sw
            && s == other.s
            && se == other.se
            && next == other.next
        ;
    }

    template<auto Member> 
    bool eq_except_for(const PreClause& other) const {
        #define check_field(field) \
            ((Member == &PreClause::field) ? (this->field != other.field) : (this->field == other.field))
        return check_field(nw)
            && check_field(n)
            && check_field(ne)
            && check_field(w)
            && check_field(c)
            && check_field(e)
            && check_field(sw)
            && check_field(s)
            && check_field(se)
            && check_field(next)
        ;
        #undef check_field
    }

    #define create_clause_transformer(name, map_nw, map_n, map_ne, map_w, map_c, map_e, map_sw, map_s, map_se, map_next) \
        PreClause name() const { \
            PreClause out = PreClause(real_clause); \
            out.map_nw = nw; \
            out.map_n = n; \
            out.map_ne = ne; \
            out.map_w = w; \
            out.map_c = c; \
            out.map_e = e; \
            out.map_sw = sw; \
            out.map_s = s; \
            out.map_se = se; \
            out.map_next = next; \
            return out; \
        }
    create_clause_transformer(flip_horizontal, ne, n, nw, e, c, w, se, s, sw, next)
    create_clause_transformer(flip_vertical, sw, s, se, w, c, e, nw, n, ne, next)
    create_clause_transformer(rotate_left, sw, w, nw, s, c, n, se, e, ne, next)
    create_clause_transformer(rotate_right, ne, e, se, n, c, s, nw, w, sw, next)
    create_clause_transformer(rotate_180, se, s, sw, e, c, w, ne, n, nw, next)
    create_clause_transformer(flip_diagonal, nw, w, sw, n, c, s, ne, e, se, next)
    create_clause_transformer(flip_anti_diagonal, se, e, ne, s, c, n, sw, w, nw, next)
    #undef create_clause_transformer

};


class Preprocessor {

private:

    ULP& ulp;

public:

    Preprocessor(ULP& ulp) : ulp(ulp) {
        // do nothing
    }

private:

    const char* current_step;

    [[noreturn]] void contradiction(const char* format, ...) {
        printf("Contradiction found in preprocessing (in step %s)\n", current_step);
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        exit(EXIT_SUCCESS);
    }

    bool changes_made = false;

    #define start_step() \
        do { \
            current_step = __func__; \
            debug(2, printf("Running step %s\n", current_step)); \
            inc_debug_depth(); \
        } while (0);

    #define end_step() \
        do { \
            dec_debug_depth(); \
            current_step = nullptr; \
        } while (0)

    void alias_variable(Variable& var, Variable& alias_to) {
        // this isn't required but it keeps variable numbers down
        if (alias_to.number > ulp.max_states && var.number > alias_to.number) {
            alias_variable(alias_to, var);
            return;
        }
        debug(2, printf("Aliasing variable %zu to %zu\n", var.number, alias_to.number));
        changes_made = true;
        Variable* new_var = (Variable*)safe_malloc(sizeof(Variable) + (var.use_count + alias_to.use_count) * sizeof(VarUse));
        new_var->number = var.number;
        new_var->value = V_UNKNOWN;
        new_var->use_count = var.use_count + alias_to.use_count;
        memcpy(new_var->uses, &(var.uses), var.use_count * sizeof(VarUse));
        memcpy(new_var->uses + var.use_count, &(alias_to.uses), alias_to.use_count * sizeof(VarUse));
        for (size_t i = 0; i < ulp.grid.total_size; i++) {
            if (ulp.grid.data[i] == var.number || ulp.grid.data[i] == alias_to.number) {
                ulp.grid.data[i] = new_var->value;
            }
        }
        for (size_t i = 0; i < ulp.clause_count; i++) {
            Clause& clause = *ulp.clauses[i];
            for (size_t i = 0; i < clause.var_count; i++) {
                if (clause.vars[i] == &var || clause.vars[i] == &alias_to) {
                    clause.vars[i] = new_var;
                }
            }
            init_clause_implication_tr(clause);
        }
        ulp.vars[var.number] = new_var;
        safe_free(&var);
        for (size_t i = alias_to.number; i <= ulp.var_count - 1; i++) {
            ulp.vars[i] = ulp.vars[i + 1];
            ulp.vars[i]->number--;
        }
        ulp.var_count--;
        safe_free(&alias_to);
    }

    void remove_var_use(Variable& var, size_t i) {
        changes_made = true;
        for (size_t j = i; j < var.use_count - 1; j++) {
            var.uses[j] = var.uses[j + 1];
        }
        var.use_count--;
    }

    void remove_clause(Clause& clause) {
        changes_made = true;
        debug(2, printf("Removing clause %zu\n", clause.number));
        for (size_t i = 0; i < clause.var_count; i++) {
            Variable& var = *clause.vars[i];
            for (size_t j = 0; j < var.use_count; j++) {
                if (var.uses[j].clause == &clause) {
                    remove_var_use(var, i);
                }
            }
        }
        for (size_t i = clause.number; i <= ulp.clause_count - 1; i++) {
            ulp.clauses[i] = ulp.clauses[i + 1];
            ulp.clauses[i]->number--;
        }
        ulp.clause_count--;
        safe_free(&clause);
    }

    void normalize_cells() {
        start_step();
        // runs several normalization steps
        for (size_t i = 1; i <= ulp.var_count; i++) {
            Variable& var = *ulp.vars[i];
            // if the cell has a known value then set it to the corresponding cell
            if (var.value != V_UNKNOWN) {
                alias_variable(var, *ulp.vars[varvalue_to_cell(var.value)]);
                continue;
            }
            // see if we can combine together any uses
            for (size_t i = 0; i < var.use_count; i++) {
                for (size_t j = i + 1; j < var.use_count; j++) {
                    if (var.uses[i].clause == var.uses[j].clause) {
                        var.uses[i].change += var.uses[j].change;
                        remove_var_use(var, j);
                    }
                }
            }
        }
        end_step();
    }

    void normalize_clauses() {
        start_step();
        // unsure what this is yet
        end_step();
    }

    void normalize() {
        normalize_cells();
        normalize_clauses();
    }

    bool internal_check_implications(Variable& var, VarValue value) {
        debug(2, printf("Checking implications for variable %zu = %i\n", var.number, value));
        inc_debug_depth();
        current_stack.push_frame();
        bool out = set_variable_and_propagate(var, value, true);
        current_stack.pop_and_undo_frame();
        dec_debug_depth();
        return out;
    }

    void check_variable_implications() {
        start_step();
        // go through each cell and try setting it to a value
        // if it's a contradiction, then it must be the OTHER value
        // so we can set it to that!
        for (size_t i = 1; i <= ulp.var_count; i++) {
            Variable& var = *ulp.vars[i];
            if (var.value != V_UNKNOWN) {
                continue;
            }
            bool can_be_0 = internal_check_implications(var, cell_to_varvalue(0));
            bool can_be_1 = internal_check_implications(var, cell_to_varvalue(1));
            debug(2, printf("Implications for variable %zu: can_be_0 = %i, can_be_1 = %i\n", var.number, can_be_0, can_be_1));
            if (!can_be_0 && !can_be_1) {
                contradiction("Variable %zu is unsettable", var.value);
            } else if (!can_be_0 && can_be_1) {
                changes_made = true;
                set_variable_and_propagate(var, cell_to_varvalue(0), true);
                // reset the stack length to fix dangling pointers caused by set_variable and save space
                current_stack.len = 0;
            } else if (can_be_0 && !can_be_1) {
                changes_made = true;
                set_variable_and_propagate(var, cell_to_varvalue(0), true);
                // reset the stack length to fix dangling pointers caused by set_variable and save space
                current_stack.len = 0;
            } else {
                // do nothing
            }
        }
        end_step();
    }

    void alias_duplicate_variables() {
        start_step();
        // if 2 variables have exactly the same arrangement of clauses
        // then alias them together
        for (size_t i = 1; i <= ulp.var_count; i++) {
            Variable& x = *ulp.vars[i];
            for (size_t j = 1; j < i; j++) {
                Variable& y = *ulp.vars[j];
                if (x.use_count != y.use_count) {
                    continue;
                }
                bool found = false;
                for (size_t k = 0; k < x.use_count; k++) {
                    VarUse& x_use = x.uses[k];
                    VarUse& y_use = y.uses[k];
                    if (!(x_use.clause == y_use.clause && x_use.change == y_use.change)) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    alias_variable(x, y);
                }
            }
        }
        end_step();
    }

    void remove_known_clauses() {
        start_step();
        // removes all clauses that are fully made of known cells
        for (size_t i = 0; i < ulp.clause_count; i++) {
            Clause& clause = *ulp.clauses[i];
            bool found = false;
            for (size_t j = 0; j < clause.var_count; j++) {
                if (clause.vars[j]->value == V_UNKNOWN) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                current_stack.push_frame();
                bool value = check_clause_implications(clause);
                current_stack.pop_and_undo_frame();
                if (value == false) {
                    contradiction("Clause %zu is UNSAT immediately", i);
                } else {
                    remove_clause(clause);
                }
            }
        }
        end_step();
    }

    void merge_duplicate_or_similar_clauses() {
        start_step();
        // looks for clauses that are exactly the same and merges them
        // also looks for clauses where they are the same except for the next value
        // which means that the next cells can be aliased together!
        // when this happens the next pass will take care of merging the clauses
        Transformations valid = rule::get_identity_transforms(ulp);
        for (size_t i = 0; i < ulp.clause_count; i++) {
            PreClause x = PreClause(*ulp.clauses[i]);
            for (size_t j = 0; j < i; j++) {
                PreClause y = PreClause(*ulp.clauses[j]);
                // no transformation
                if (x == y) {
                    debug(2, printf("Match found: clause %zu is equal to clause %zu with no transformation\n", x.get_number(), y.get_number()));
                    remove_clause(y.real_clause);
                    continue;
                } else if (x.eq_except_for<&PreClause::next>(y)) {
                    debug(2, printf("Match found: clause %zu has same next as clause %zu with no transformation\n", x.get_number(), y.get_number()));
                    alias_variable(*x.next, *y.next);
                    continue;
                }
                // macro for the rest of the transformations
                #define check(transform) \
                    if (valid.transform) { \
                        PreClause y2 = y.transform(); \
                        if (x == y2) { \
                            debug(2, printf("Match found: clause %zu is equal to clause %zu with transformation "#transform"\n", x.get_number(), y2.get_number())); \
                            remove_clause(y2.real_clause); \
                            continue; \
                        } else if (x.eq_except_for<&PreClause::next>(y2)) { \
                            debug(2, printf("Match found: clause %zu has same next as clause %zu with transformation "#transform"\n", x.get_number(), y2.get_number())); \
                            alias_variable(*x.next, *y2.next); \
                            continue; \
                        } \
                    }
                check(flip_horizontal);
                check(flip_vertical);
                check(rotate_left);
                check(rotate_right);
                check(rotate_180);
                check(flip_diagonal);
                check(flip_anti_diagonal);
                #undef check
            }
        }
        end_step();
    }

    #undef start_step
    #undef end_step

public:

    // returns whether it was solved in preprocessing
    bool preprocess() {
        debug(2,
            printf("Preprocessing\n");
        );
        bool found = false;
        for (size_t i = 0; i < MAX_PREPROCESSING_ITERATIONS; i++) {
            debug(2, printf("Running preprocessing iteration %zu\n", i));
            changes_made = false;
            inc_debug_depth();
            normalize();
            check_variable_implications();
            normalize();
            alias_duplicate_variables();
            normalize();
            remove_known_clauses();
            normalize();
            merge_duplicate_or_similar_clauses();
            normalize();
            dec_debug_depth();
            if (!changes_made) {
                found = true;
                break;
            }
        }
        if (!found) {
            unexpected_error("Preprocessing did not finish");
        }
        if (ulp.var_count == 0) {
            solutions::check(*search, true);
            return true;
        }
        // move back the stack pointer to save space
        current_stack.len = 0;
        return false;
    }

};

static void preprocess() {
    Preprocessor value = Preprocessor(search->ulp);
    value.preprocess();
}


}
