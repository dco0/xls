
#pragma once

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "../base/base.cpp"
#include "../base/search.cpp"


// forward declarations

static bool set_variable(Variable* var, VarValue value);

static void unset_variable(Variable* var);


typedef struct StackEntry {
    bool is_first_in_frame : 1;
    Variable* var;
    VarValue value;
} StackEntry;

static inline void print_stack_entry(StackEntry* entry) {
    if (entry == NULL) {
        real_printf("<null pointer>\n");
        return;
    }
    real_printf("variable set: number = %zu, value = %i", entry->var->number, entry->value);
    if (entry->is_first_in_frame) {
        real_printf(", first in frame");
    }
    real_printf("\n");
}

// returns false if contradiction, true if no contradiction
static inline bool apply_stack_entry(StackEntry* entry) {
    #if DEBUG >= 4
        printf("Applying stack entry: ");
        print_stack_entry(entry);
    #endif
    if (!set_variable(entry->var, entry->value)) {
        return false;
    }
    search_data.set_unknown_cells++;
    return true;
}

static inline void undo_stack_entry(StackEntry* entry) {
    #if DEBUG >= 4
        printf("Popping stack entry: ");
        print_stack_entry(entry);
    #endif
    unset_variable(entry->var);
    search_data.set_unknown_cells--;
}

typedef struct BigStackData {
    size_t capacity;
    StackEntry* data;
} BigStackData;

// todo: test this for optimal performance
constexpr size_t STACK_INLINE_SIZE = 4;
// constexpr size_t STACK_INLINE_SIZE = (sizeof(BigStackData) / sizeof(StackEntry))

typedef struct Stack {
    union {
        StackEntry small_data[STACK_INLINE_SIZE];
        BigStackData big_data;
    };
    size_t len;
    bool is_big : 1;
    bool next_entry_is_first_in_frame : 1;
} Stack;

Stack* current_stack;

static inline Stack* create_stack(void) {
    Stack* out = (Stack*)safe_malloc(sizeof(Stack));
    out->len = 0;
    out->next_entry_is_first_in_frame = true;
    out->is_big = false;
    return out;
}

static inline void destroy_stack(Stack* stack) {
    if (stack->is_big) {
        safe_free(stack->big_data.data);
    }
    safe_free(stack);
}

// RETURNS NULL POINTER if the entry does not exist
static inline StackEntry* get_stack_entry(Stack* stack, size_t i) {
    if (i >= stack->len) {
        return NULL;
    }
    if (stack->is_big) {
        return &(stack->big_data.data[i]);
    } else {
        return &(stack->small_data[i]);
    }
}

static inline __attribute__((used)) void print_stack(Stack* stack) {
    printf("Stack:\n");
    for (size_t i = 0; i < stack->len; i++) {
        printf(INDENT "Entry: ");
        print_stack_entry(get_stack_entry(stack, i));
    }
}

// no dedicated "push" function because that functionality
// is provided by set_cell and set_tr

static inline StackEntry* create_new_stack_entry(Stack* stack, [[maybe_unused]] bool is_explicit) {
    StackEntry* out;
    if (stack->is_big) {
        BigStackData* big_data = &(stack->big_data);
        if (stack->len == big_data->capacity) {
            big_data->capacity <<= 1;
            // check for overflow
            if (big_data->capacity == 0) {
                big_data->capacity -= 1;
            }
            big_data->data = (StackEntry*)safe_realloc(big_data->data, big_data->capacity * sizeof(StackEntry));
        }
        stack->len++;
        out = &(big_data->data[stack->len - 1]);
    } else if (stack->len == STACK_INLINE_SIZE) {
        size_t capacity = STACK_INLINE_SIZE * 2;
        StackEntry* data_ptr = (StackEntry*)safe_malloc(capacity * sizeof(StackEntry));
        memcpy(data_ptr, &(stack->small_data), STACK_INLINE_SIZE * sizeof(StackEntry));
        stack->is_big = true;
        BigStackData* big_data = &(stack->big_data);
        big_data->capacity = capacity;
        big_data->data = data_ptr;
        stack->len++;
        out = &(big_data->data[stack->len - 1]);
    } else {
        stack->len++;
        out = &(stack->small_data[stack->len - 1]);
    }
    if (stack->next_entry_is_first_in_frame) {
        out->is_first_in_frame = true;
        stack->next_entry_is_first_in_frame = false;
    } else {
        out->is_first_in_frame = false;
    }
    // out->is_explicit = is_explicit;
    return out;
}

// RETURNS NULL POINTER if there is nothing to pop
static inline StackEntry* pop_stack_for_entry(Stack* stack) {
    if (stack->len == 0) {
        return NULL;
    }
    StackEntry* out;
    if (stack->is_big) {
        out = &(stack->big_data.data[stack->len - 1]);
    } else {
        out = &(stack->small_data[stack->len - 1]);
    }
    stack->len--;
    return out;
}

static inline bool pop_stack(Stack* stack) {
    StackEntry* entry = pop_stack_for_entry(stack);
    if (entry == NULL) {
        return false;
    }
    // maybe check for contradiction here?
    // it really shouldn't trigger the MAXPOP
    // but who knows
    undo_stack_entry(entry);
    return true;
}

static inline void push_stack_frame(Stack* stack) {
    debug(4, printf("Pushing frame\n"));
    stack->next_entry_is_first_in_frame = true;
}

static inline void pop_stack_frame(Stack* stack) {
    debug(4, printf("Popping frame\n"));
    while (stack->len > 0) {
        bool done = get_stack_entry(stack, stack->len - 1)->is_first_in_frame;
        pop_stack(stack);
        if (done) {
            break;
        }
    }
    debug(4, printf("Pop complete\n"));
}

// returns false if contradiction, true if no contradiction
static inline bool set_variable_and_push(Variable* var, VarValue value, bool is_explicit) {
    if (!set_variable(var, value)) {
        return false;
    }
    StackEntry* out = create_new_stack_entry(current_stack, is_explicit);
    out->var = var;
    out->value = value;
    return true;
}
