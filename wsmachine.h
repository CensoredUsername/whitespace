#ifndef WSMACHINE_H
#define WSMACHINE_H

#include "wstypes.h"

#include <stdio.h>
#include <windows.h>

typedef struct {
    ws_int key;
    ws_int value;
    char initialized;
} ws_heap_entry;

typedef struct {
    size_t size;
    size_t length;
    ws_heap_entry *entries;
} ws_heap;

typedef struct {
    size_t size;
    size_t length;
    ws_int *entries;
} ws_stack;

typedef struct {
    size_t size;
    size_t length;
    size_t *entries;
} ws_callstack;

/* The heap, a very simple hash table implementation
 * It only supports inserting and getting values
 * 
 * The details of this implementation have been inspired by cpythons dict implementation
 * 
 */
#define WS_HEAP_SIZE 8
#define WS_HEAP_RESIZE_FACTOR 4
#define WS_RESIZE_TIME(length, size) (((length)+1)*3 > (size)*2)
#define WS_PERTURB_SHIFT 5

static ws_heap *ws_heap_initialize() {
    ws_heap *result = (ws_heap *)malloc(sizeof(ws_heap));
    result->size = WS_HEAP_SIZE;
    result->length = 0;
    result->entries = (ws_heap_entry *)malloc(sizeof(ws_heap_entry) * WS_HEAP_SIZE);
    for(size_t i = 0; i < WS_HEAP_SIZE; i++) {
        result->entries[i].initialized = 0;
    }
    return result;
}

static void ws_heap_free(ws_heap *const table) {
    free(table->entries);
    free(table);
}

static void ws_heap_insert(ws_heap *const table, const ws_int key, const ws_int value) {
    //find a free spot and insert our value
    int hash = ws_int_hash(key);
    int perturb = hash / table->size;
    size_t position = hash % table->size;
    while (table->entries[position].initialized &&
           !ws_int_cmp(table->entries[position].key, key)) {

        position = (position * 5 + 1 + perturb) % table->size;
        perturb >>= WS_PERTURB_SHIFT;
    }
    table->entries[position].key = key;
    table->entries[position].value = value;
    table->entries[position].initialized = 1;
    table->length++;
}

static void ws_heap_set(ws_heap *const table, const ws_int key, const ws_int value) {
    // check if we'e getting too large, and resize
    if (WS_RESIZE_TIME(table->length, table->size)) {

        // oh boy, resizing. save the old table size and 
        ws_heap_entry *old = table->entries;
        size_t old_size = table->size;

        // create a new array to hold everything
        table->size *= WS_HEAP_RESIZE_FACTOR;
        table->length = 0;
        table->entries = (ws_heap_entry *)malloc(sizeof(ws_heap_entry) * table->size);

        // set everyting to uninitialized
        for(size_t i = 0; i < table->size; i++) {
            table->entries[i].initialized = 0;
        }

        // and populate the new array with the old values
        for(size_t i = 0; i < old_size; i++) {
            if (old[i].initialized) {
                ws_heap_insert(table, old[i].key, old[i].value);
            }
        }

        // finally, free the old array
        free(old);
    }
    // actual insertion
    ws_heap_insert(table, key, value);
}

static ws_int ws_heap_get(const ws_heap *const table, const ws_int key) {
    //find the spot key and return the value
    int hash = ws_int_hash(key);
    int perturb = hash / table->size;
    size_t position = hash % table->size;
    while (table->entries[position].initialized) {
        if (ws_int_cmp(table->entries[position].key, key)) {
            return table->entries[position].value;
        }
        position = (position * 5 + 1 + perturb) % table->size;
        perturb >>= WS_PERTURB_SHIFT;
    }
    printf("Tried to look up value in the heap at %d which did not exist\n", ws_int_to_int(key));
    exit(EXIT_FAILURE);
}



/* The stack, a simple resizing array */
#define WS_STACK_SIZE 1024
#define WS_STACK_RESIZE_FACTOR 2

static ws_stack *ws_stack_initialize() {
    ws_stack *result = (ws_stack *)malloc(sizeof(ws_stack));
    result->size = WS_STACK_SIZE;
    result->length = 0;
    result->entries = (ws_int *)malloc(sizeof(ws_int) * WS_STACK_SIZE);
    return result;
}

static void ws_stack_free(ws_stack *const stack) {
    free(stack->entries);
    free(stack);
}

static void ws_stack_print(ws_stack *const stack) {
    printf("stack contents with length %d:\n", stack->length);
    for (size_t i = 0; i < stack->length; i++) {
        printf("%X\n", ws_int_to_int(stack->entries[i]));
    }
}


/* same for the callstack */
static ws_callstack *ws_callstack_initialize() {
    ws_callstack *result = (ws_callstack *)malloc(sizeof(ws_callstack));
    result->size = WS_STACK_SIZE;
    result->length = 0;
    result->entries = (size_t *)malloc(sizeof(size_t) * WS_STACK_SIZE);
    return result;
}

static void ws_callstack_free(ws_callstack *const stack) {
    free(stack->entries);
    free(stack);
}



/* and implementations of all commands */
static void ws_command_push(ws_stack *const stack, const ws_int input) {
    if (stack->length == stack->size) {
        stack->size *= WS_STACK_RESIZE_FACTOR;
        stack->entries = (ws_int *)realloc(stack->entries, sizeof(ws_int) * stack->size);
    }
    stack->entries[stack->length++] = input;
}

static void ws_command_duplicate(ws_stack *const stack) {
    ws_command_push(stack, stack->entries[stack->length-1]);
}

static void ws_command_copy(ws_stack *const stack, const ws_int index) {
    int i = ws_int_to_int(index);
    if (i < 0 || i >= stack->length) {
        printf("Tried to copy from position not on stack\n");
        exit(EXIT_FAILURE);
    }
    ws_command_push(stack, stack->entries[i]);
}

static void ws_command_swap(ws_stack *const stack) {
    if(stack->length < 2) {
        printf("need at least two items on the stack to swap\n");
        exit(EXIT_FAILURE);
    }
    ws_int temp = stack->entries[stack->length-1];
    stack->entries[stack->length-1] = stack->entries[stack->length-2];
    stack->entries[stack->length-2] = temp;
}

static ws_int ws_command_discard(ws_stack *const stack) {
    if(!stack->length) {
        printf("tried to pop from empty stack\n");
        exit(EXIT_FAILURE);
    }
    return stack->entries[--(stack->length)];
}

static void ws_command_slide(ws_stack *const stack, const ws_int amount) {
    ws_int tokeep = stack->entries[stack->length-1];
    int i = ws_int_to_int(amount);
    if (i < 0 || i >= stack->length) {
        printf("Tried to slide amount not on stack\n");
        exit(EXIT_FAILURE);
    }
    stack->length -= i;
    stack->entries[stack->length-1] = tokeep;
}

static void ws_command_add(ws_stack *const stack) {
    if(stack->length < 2) {
        printf("need at least two items on the stack to add\n");
        exit(EXIT_FAILURE);
    }
    stack->entries[stack->length-2] = ws_int_add(stack->entries[stack->length-2], stack->entries[stack->length-1]);
    stack->length--;
}

static void ws_command_subtract(ws_stack *const stack) {
    if(stack->length < 2) {
        printf("need at least two items on the stack to subtract\n");
        exit(EXIT_FAILURE);
    }
    stack->entries[stack->length-2] = ws_int_subtract(stack->entries[stack->length-2], stack->entries[stack->length-1]);
    stack->length--;
}

static void ws_command_multiply(ws_stack *const stack) {
    if(stack->length < 2) {
        printf("need at least two items on the stack to multiply\n");
        exit(EXIT_FAILURE);
    }
    stack->entries[stack->length-2] = ws_int_multiply(stack->entries[stack->length-2], stack->entries[stack->length-1]);
    stack->length--;
}

static void ws_command_divide(ws_stack *const stack) {
    if(stack->length < 2) {
        printf("need at least two items on the stack to divide\n");
        exit(EXIT_FAILURE);
    }
    stack->entries[stack->length-2] = ws_int_divide(stack->entries[stack->length-2], stack->entries[stack->length-1]);
    stack->length--;
}

static void ws_command_modulo(ws_stack *const stack) {
    if(stack->length < 2) {
        printf("need at least two items on the stack to modulo\n");
        exit(EXIT_FAILURE);
    }
    stack->entries[stack->length-2] = ws_int_modulo(stack->entries[stack->length-2], stack->entries[stack->length-1]);
    stack->length--;
}

static void ws_command_set(ws_stack *const stack, ws_heap *const heap) {
    ws_int value = ws_command_discard(stack);
    ws_int key = ws_command_discard(stack);
    ws_heap_set(heap, key, value);
}

static void ws_command_get(ws_stack *const stack, ws_heap *const heap) {
    ws_int key = ws_command_discard(stack);
    ws_command_push(stack, ws_heap_get(heap, key));
}

static size_t ws_command_call(ws_callstack *const callstack, const size_t dest, const size_t next_index) {
    if (callstack->length == callstack->size) {
        callstack->size *= WS_STACK_RESIZE_FACTOR;
        callstack->entries = (size_t *)realloc(callstack->entries, sizeof(size_t) * callstack->size);
    }
    callstack->entries[callstack->length++] = next_index;
    return dest;
}

static size_t ws_command_jump(const size_t dest) {
    return dest;
}

static size_t ws_command_jumpifzero(ws_stack *const stack, const size_t dest, const size_t next_index) {
    if (ws_int_to_int(ws_command_discard(stack)) == 0) {
        return dest;
    }
    return next_index;
}

static size_t ws_command_jumpifnegative(ws_stack *const stack, const size_t dest, const size_t next_index) {
    if (ws_int_to_int(ws_command_discard(stack)) < 0) {
        return dest;
    }
    return next_index;
}

static size_t ws_command_endsubroutine(ws_callstack *const callstack) {
    if (!callstack->length) {
        printf("can't end subroutine without calls on the callstack\n");
        exit(EXIT_FAILURE);
    }
    return callstack->entries[--(callstack->length)];
}

static void ws_command_endprogram(ws_callstack *const callstack) {
    if (callstack->length) {
        printf("warning: attempted to end the program with a non-empty callstack\n");
    }
}

static void ws_command_printchar(ws_stack *const stack) {
    putchar(ws_int_to_int(ws_command_discard(stack)));
}

static void ws_command_printnum(ws_stack *const stack){
    ws_int_print(ws_command_discard(stack));
}

static void ws_command_inputchar(ws_stack *const stack, ws_heap *const heap){
    int i = getchar();
    ws_heap_set(heap, stack->entries[stack->length-1], ws_int_from_int(i));
}

static void ws_command_inputnum(ws_stack *const stack, ws_heap *const heap) {
    ws_heap_set(heap, stack->entries[stack->length-1], ws_int_input());
}

void ws_execute(const ws_compiled *const program) {
    
    // initialize the heap
    ws_heap *heap = ws_heap_initialize();

    // and the stack
    ws_stack *stack = ws_stack_initialize();

    // and the callstack
    ws_callstack *callstack = ws_callstack_initialize();

    size_t next_index = 0;
    ws_command *current_command;

    int exitcode = 0;

    while (!exitcode) {
        current_command = program->commands + next_index;
        next_index++;
        /*Sleep(500);
        ws_stack_print(stack);
        printf("executing command type: ");
        ws_string_print(ws_command_names[current_command->type]);
        printf("\n");*/
        switch (current_command->type) {

            case push:
                ws_command_push(stack, current_command->parameter);
                break;

            case duplicate:
                ws_command_duplicate(stack);
                break;

            case copy:
                ws_command_copy(stack, current_command->parameter);
                break;

            case swap:
                ws_command_swap(stack);
                break;

            case discard:
                ws_command_discard(stack);
                break;

            case slide:
                ws_command_slide(stack, current_command->parameter);
                break;

            case add:
                ws_command_add(stack);
                break;

            case subtract:
                ws_command_subtract(stack);
                break;

            case multiply:
                ws_command_multiply(stack);
                break;

            case divide:
                ws_command_divide(stack);
                break;

            case modulo:
                ws_command_modulo(stack);
                break;

            case set:
                ws_command_set(stack, heap);
                break;

            case get:
                ws_command_get(stack, heap);
                break;

            case label:
                break;

            case call:
                next_index = ws_command_call(callstack, current_command->jumpoffset, next_index);
                break;

            case jump:
                next_index = ws_command_jump(current_command->jumpoffset);
                break;

            case jumpifzero:
                next_index = ws_command_jumpifzero(stack, current_command->jumpoffset, next_index);
                break;

            case jumpifnegative:
                next_index = ws_command_jumpifnegative(stack, current_command->jumpoffset, next_index);
                break;

            case endsubroutine:
                next_index = ws_command_endsubroutine(callstack);
                break;

            case endprogram:
                ws_command_endprogram(callstack);
                exitcode = 1;
                break;

            case printchar:
                ws_command_printchar(stack);
                break;

            case printnum:
                ws_command_printnum(stack);
                break;

            case inputchar:
                ws_command_inputchar(stack, heap);
                break;

            case inputnum:
                ws_command_inputnum(stack, heap);
                break;

            default:
                exitcode = 2;
                break;
        }

        if (next_index >= program->length && !exitcode) {
            exitcode = 3;
        }

    }
    
    ws_callstack_free(callstack);
    ws_stack_free(stack);
    ws_heap_free(heap);

    switch (exitcode) {
        case 1: //clean exit
            exit(EXIT_SUCCESS);
            break;
        case 2: //unsupported command type
            printf("invalid command type\n");
            exit(EXIT_FAILURE);
            break;
        case 3: //code index pointer out of bounds
            printf("code index out of bounds\n");
            exit(EXIT_FAILURE);
            break;
    }

}

#endif