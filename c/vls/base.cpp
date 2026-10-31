
#pragma once

#include <stddef.h>

#include "../base/base.cpp"
#include "../base/search.cpp"


typedef struct StackEntry {
    bool is_first_in_frame : 1;
    Variable* var;
} StackEntry;

#if MULTI_RULE
    static const char* bound_trs_names[512];
    size_t tr_to_bound_tr[512];
#endif

static inline void print_stack_entry(StackEntry* entry) {
    if (entry == NULL) {
        real_printf("<null pointer>");
        return;
    } else {
        Cell* cell = &(state.grid[entry->data.cell_set]);
        real_printf("variable set %zu", );
    }
    // if (entry->is_explicit) {
    //     real_printf(", explicit");
    // } else {
    //     real_printf(", not explicit");
    // }
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
    #if MULTI_RULE
    if (entry->type == STACKENTRY_TYPE_CELL_SET) {
    #endif
        Cell* cell = &(state.grid[entry->data.cell_set]);
        CellValue value = ((CellValue*)INITIAL_STATES)[cell->index];
        unsafe_set_cell_value(cell, value);
        state.set_unknown_cells++;
        #ifdef MAXPOP
        if (cell->t == 0) {
            if (value == ON && cell->value != ON) {
                state.phase_0_pop++;
                if (state.phase_0_pop > MAXPOP) {
                    return false;
                }
            } else if (value != ON && cell->value == ON) {
                state.phase_0_pop--;
            }
        }
        #endif
    #if MULTI_RULE
    } else {
        BoundTransition value = entry->data.rule_change;
        unsafe_set_tr(value >> 4, value & 3);
    }
    #endif
    return true;
}

// returns false if contradiction, true if no contradiction
static inline bool undo_stack_entry(StackEntry* entry) {
    #if DEBUG >= 4
        printf("Popping stack entry: ");
        print_stack_entry(entry);
    #endif
    #if MULTI_RULE
    if (entry->type == STACKENTRY_TYPE_CELL_SET) {
    #endif
        Cell* cell = &(state.grid[entry->data.cell_set]);
        CellValue value = ((CellValue*)INITIAL_STATES)[cell->index];
        unsafe_set_cell_value(cell, value);
        state.set_unknown_cells--;
        #ifdef MAXPOP
        if (cell->t == 0) {
            if (value == ON && cell->value != ON) {
                state.phase_0_pop++;
                if (state.phase_0_pop > MAXPOP) {
                    return false;
                }
            } else if (value != ON && cell->value == ON) {
                state.phase_0_pop--;
            }
        }
        #endif
    #if MULTI_RULE
    } else {
        BoundTransition value = entry->data.rule_change;
        unsafe_set_tr(value >> 4, (value >> 2) & 3);
    }
    #endif
    return true;
}

typedef PrefixIndex StackIndex;

typedef struct BigStackData {
    StackIndex capacity;
    StackEntry* data;
} BigStackData;

// todo: test this for optimal performance
#define STACK_INLINE_SIZE 4
// #define STACK_INLINE_SIZE ((Index)(sizeof(BigStackData) / sizeof(StackEntry)))

typedef struct Stack {
    union {
        StackEntry small_data[STACK_INLINE_SIZE];
        BigStackData big_data;
    };
    StackIndex len;
    bool is_big : 1;
    bool next_entry_is_first_in_frame : 1;
} Stack;

Stack* current_stack;

static inline Stack* create_stack(void) {
    Stack* out = malloc(sizeof(Stack));
    out->len = 0;
    out->next_entry_is_first_in_frame = true;
    out->is_big = false;
    return out;
}

static inline void destroy_stack(Stack* stack) {
    if (stack->is_big) {
        free(stack->big_data.data);
    }
    free(stack);
}

// RETURNS NULL POINTER if the entry does not exist
static inline StackEntry* get_stack_entry(Stack* stack, StackIndex i) {
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
    for (Index i = 0; i < stack->len; i++) {
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
            big_data->data = safe_realloc(big_data->data, big_data->capacity * sizeof(StackEntry));
        }
        stack->len++;
        out = &(big_data->data[stack->len - 1]);
    } else if (stack->len == STACK_INLINE_SIZE) {
        StackIndex capacity = ((StackIndex)1) << (sizeof(unsigned int) * CHAR_BIT - __builtin_clz(STACK_INLINE_SIZE - 1) + 1);
        StackEntry* data_ptr = malloc(capacity * sizeof(StackEntry));
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
    DPRINTF4("Pushing frame\n");
    stack->next_entry_is_first_in_frame = true;
}

static inline void pop_stack_frame(Stack* stack) {
    DPRINTF4("Popping frame\n");
    while (stack->len > 0) {
        bool done = get_stack_entry(stack, stack->len - 1)->is_first_in_frame;
        pop_stack(stack);
        if (done) {
            break;
        }
    }
    DPRINTF4("Pop complete\n");
    DPRINTGRID4();
}


// set a cell to a value, taking care of edges and filters but not propagating implications
// returns true if no contradiction, false if contradiction
// also pushes an entry to the stack
static inline bool set_cell(Cell* cell, CellValue value, bool is_explicit) {
    if (cell->value != UNKNOWN && cell->value != value) {
        DPRINTF4("Contradiction (previous value mismatch, both known and unequal, t = %i, x = %i, y = %i, value = %i, prev_value = %i)\n", cell->t, cell->x, cell->y, value, cell->value);
        return false;
    } else if (cell->settable == NOT_SETTABLE) {
        return true;
    } else if (cell->value == value) {
        return true;
    } else if (cell->x < PADDING
            || cell->x > state.width - PADDING - 1
            || cell->y < PADDING
            || cell->y > state.height - PADDING - 1) {
        DPRINTF4("Contradiction (out of bounds, t = %i, x = %i, y = %i, value = %i, prev_value = %i)\n", cell->t, cell->x, cell->y, value, cell->value);
        return false;
    }
    DPRINTF4("Setting cell: t = %i, x = %i, y = %i, index = %i, value = %i, prev_value = %i\n", cell->t, cell->x, cell->y, cell->index, value, cell->value);
    unsafe_set_cell_value(cell, value);
    state.set_unknown_cells++;
    #ifdef MAXPOP
        if (cell->t == 0) {
            if (value == ON && cell->value != ON) {
                state.phase_0_pop++;
                if (state.phase_0_pop > MAXPOP) {
                    return false;
                }
            } else if (value != ON && cell->value == ON) {
                state.phase_0_pop--;
            }
        }
    #endif
    StackEntry* entry = create_new_stack_entry(current_stack, is_explicit);
    entry->type = STACKENTRY_TYPE_CELL_SET;
    entry->data.cell_set = cell->index;
    #if DEBUG >= 4
        print_stack(current_stack);
    #endif
    return true;
}
