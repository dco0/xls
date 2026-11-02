
#pragma once

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "../base/base.cpp"


namespace vls {


// forward declarations

static bool set_variable(Variable& var, VarValue value);

static void unset_variable(Variable& var);


class Stack {
    
public:

    struct Entry {
        bool is_first_in_frame;
        VarValue value;
        Variable* var;
    };

    static void print_entry(Entry& entry) {
        real_printf("variable set: number = %zu, value = %i", entry.var->number, entry.value);
        if (entry.is_first_in_frame) {
            real_printf(", first in frame");
        }
        real_printf("\n");
    }

    // returns false if no contradiction, true if contradiction
    static bool apply_entry(Entry& entry) {
        debug(4,
            printf("Applying stack entry: ");
            print_entry(entry);
        );
        if (!set_variable(*entry.var, entry.value)) {
            return false;
        }
        return true;
    }

    static void undo_entry(Entry& entry) {
        debug(4,
            printf("Undoing stack entry: ");
            print_entry(entry);
        );
        unset_variable(*entry.var);
    }

    struct BigData {
        size_t capacity;
        Entry* data;
    };

    // todo: test this for optimal performance
    static constexpr size_t STACK_INLINE_SIZE = 4;
    // static constexpr size_t STACK_INLINE_SIZE = (sizeof(BigData) / sizeof(Entry));

    bool is_big : 1;
    bool is_at_top_of_frame : 1;
    size_t len : sizeof(size_t) - 2;
    union {
        Entry small_data[STACK_INLINE_SIZE];
        BigData big_data;
    };

    Stack() {
        len = 0;
        is_at_top_of_frame = true;
        is_big = false;
    }

    ~Stack() {
        if (is_big) {
            safe_free(big_data.data);
        }
    }

    Entry& get_entry(size_t i) {
        if (is_big) {
            return big_data.data[i];
        } else {
            return small_data[i];
        }
    }

    [[maybe_unused]] void print() {
        printf("Stack:\n");
        for (size_t i = 0; i < len; i++) {
            printf(INDENT "Entry: ");
            print_entry(get_entry(i));
        }
    }

    Entry& pop() {
        if (len == 0) {
            unexpected_error("Nothing to pop off of stack");
        }
        len--;
        if (is_big) {
            return big_data.data[len];
        } else {
            return small_data[len];
        }
    }

    void pop_and_undo() {
        undo_entry(pop());
    }

    void push_frame() {
        is_at_top_of_frame = true;
    }

    void pop_frame() {
        if (is_at_top_of_frame) {
            return;
        } else {
            while (len > 0) {
                bool done = get_entry(len - 1).is_first_in_frame;
                pop();
                if (done) {
                    break;
                }
            }
        }
    }
    
    void pop_and_undo_frame() {
        debug(4, printf("Popping frame\n"));
        if (is_at_top_of_frame) {
            debug(4, printf("Stack holds 0-length frame at top, doing nothing\n"));
            is_at_top_of_frame = false;
        } else {
            while (len > 0) {
                bool done = get_entry(len - 1).is_first_in_frame;
                pop_and_undo();
                if (done) {
                    break;
                }
            }
        }
        debug(4, printf("Pop complete\n"));
    }

    Entry& create_new_entry([[maybe_unused]] bool is_explicit) {
        Entry* out;
        if (is_big) {
            if (len == big_data.capacity) {
                // check for overflow
                size_t old_capacity = big_data.capacity;
                big_data.capacity <<= 1;
                if (big_data.capacity < old_capacity) {
                    big_data.capacity = SIZE_MAX;
                }
                big_data.data = (Entry*)safe_realloc(big_data.data, big_data.capacity * sizeof(Entry));
            }
            out = &(big_data.data[len]);
        } else if (len == STACK_INLINE_SIZE) {
            size_t capacity = STACK_INLINE_SIZE * 2;
            Entry* data_ptr = (Entry*)safe_malloc(capacity * sizeof(Entry));
            memcpy(data_ptr, &small_data, STACK_INLINE_SIZE * sizeof(Entry));
            is_big = true;
            big_data.capacity = capacity;
            big_data.data = data_ptr;
            out = &(big_data.data[len]);
        } else {
            out = &(small_data[len]);
        }
        len++;
        if (is_at_top_of_frame) {
            out->is_first_in_frame = true;
            is_at_top_of_frame = false;
        } else {
            out->is_first_in_frame = false;
        }
        // out->is_explicit = is_explicit;
        return *out;
    }

};


Stack current_stack;


// returns false if contradiction, true if no contradiction
static inline bool set_variable_and_push(Variable& var, VarValue value, bool is_explicit) {
    if (!set_variable(var, value)) {
        return false;
    }
    Stack::Entry& out = current_stack.create_new_entry(is_explicit);
    out.var = &var;
    out.value = value;
    return true;
}


}
