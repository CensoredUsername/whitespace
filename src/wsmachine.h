#ifndef WSMACHINE_H
#define WSMACHINE_H

#include "wstypes.h"
//#include "time.h"

//data structures for ws intepretation runtime
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

/* Forward declarations of everything used in the main loop
 */
void ws_heap_initialize(ws_heap *);
void ws_heap_finish(ws_heap *);
void ws_heap_set(ws_heap *, const ws_int *, const ws_int *);
void ws_heap_get(ws_int *, const ws_heap *, const ws_int *);

void ws_stack_initialize(ws_stack *);
void ws_stack_finish(ws_stack *);

void ws_heap_print(ws_heap *);
void ws_stack_print(ws_stack *);

void ws_callstack_initialize(ws_callstack *);
void ws_callstack_finish(ws_callstack *);

static void ws_command_push(ws_stack *, const ws_int *);
static void ws_command_duplicate(ws_stack *);
static void ws_command_copy(ws_stack *, const ws_int *);
static void ws_command_swap(ws_stack *);
static void ws_command_discard(ws_int *, ws_stack *);
static void ws_command_slide(ws_stack *, const ws_int *);
static void ws_command_add(ws_stack *);
static void ws_command_subtract(ws_stack *);
static void ws_command_multiply(ws_stack *);
static void ws_command_divide(ws_stack *);
static void ws_command_modulo(ws_stack *);
static void ws_command_set(ws_stack *, ws_heap *);
static void ws_command_get(ws_stack *, ws_heap *);
static void ws_command_call(size_t *, ws_callstack *, size_t);
static void ws_command_jump(size_t *, size_t);
static void ws_command_jumpifzero(size_t *, ws_stack *, size_t); //busy replacing output to pointers here
static void ws_command_jumpifnegative(size_t *, ws_stack *, size_t);
static void ws_command_endsubroutine(size_t *, ws_callstack *);
static void ws_command_endprogram(ws_callstack *);
static void ws_command_printchar(ws_stack *);
static void ws_command_printnum(ws_stack *);
static void ws_command_inputchar(ws_stack *, ws_heap *);
static void ws_command_inputnum(ws_stack *, ws_heap *);



/* And now the actual main loop of the program 
 */
void ws_execute(const ws_program *const program) {

    if (!(program->flags & 0x1)) {
        printf("This program has not been compiled yet");
        exit(EXIT_FAILURE);
    }
    
    // initialize the heap
    ws_heap heap;
    ws_heap_initialize(&heap);

    // and the stack
    ws_stack stack;
    ws_stack_initialize(&stack);

    // and the callstack
    ws_callstack callstack;
    ws_callstack_initialize(&callstack);

    size_t next_index = 0;
    ws_command *current_command;

    //size_t commands_executed = 0;
    //clock_t thetime = clock();

    int exitcode = 0;

    while (!exitcode) {
        current_command = program->commands + next_index;
        next_index++;
        //commands_executed++;

        switch (current_command->type) {

            case push:
                ws_command_push(&stack, &current_command->parameter);
                break;

            case duplicate:
                ws_command_duplicate(&stack);
                break;

            case copy:
                ws_command_copy(&stack, &current_command->parameter);
                break;

            case swap:
                ws_command_swap(&stack);
                break;

            case discard:
                ws_command_discard(NULL, &stack);
                break;

            case slide:
                ws_command_slide(&stack, &current_command->parameter);
                break;

            case add:
                ws_command_add(&stack);
                break;

            case subtract:
                ws_command_subtract(&stack);
                break;

            case multiply:
                ws_command_multiply(&stack);
                break;

            case divide:
                ws_command_divide(&stack);
                break;

            case modulo:
                ws_command_modulo(&stack);
                break;

            case set:
                ws_command_set(&stack, &heap);
                break;

            case get:
                ws_command_get(&stack, &heap);
                break;

            case label:
                break;

            case call:
                ws_command_call(&next_index, &callstack, current_command->jumpoffset);
                break;

            case jump:
                ws_command_jump(&next_index, current_command->jumpoffset);
                break;

            case jumpifzero:
                ws_command_jumpifzero(&next_index, &stack, current_command->jumpoffset);
                break;

            case jumpifnegative:
                ws_command_jumpifnegative(&next_index, &stack, current_command->jumpoffset);
                break;

            case endsubroutine:
                ws_command_endsubroutine(&next_index, &callstack);
                break;

            case endprogram:
                ws_command_endprogram(&callstack);
                exitcode = 1;
                break;

            case printchar:
                ws_command_printchar(&stack);
                break;

            case printnum:
                ws_command_printnum(&stack);
                break;

            case inputchar:
                ws_command_inputchar(&stack, &heap);
                break;

            case inputnum:
                ws_command_inputnum(&stack, &heap);
                break;

            default:
                exitcode = 2;
                break;
        }

        if (next_index >= program->length && !exitcode) {
            exitcode = 3;
        }

    }
    
    ws_callstack_finish(&callstack);
    ws_stack_finish(&stack);
    ws_heap_finish(&heap);

    switch (exitcode) {
        case 1: //clean exit
            //printf("time taken: %f\ncommands executed: %d\n", ((double)(clock()-thetime))/(double)CLOCKS_PER_SEC, commands_executed);
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



/* The heap, a very simple hash table implementation
 * It only supports inserting and getting values
 * 
 * The details of this implementation have been inspired by cpythons dict implementation
 * 
 */
#define WS_HEAP_SIZE 16
#define WS_HEAP_RESIZE_FACTOR 4
#define WS_HEAP_RESIZE_TIME(length, size) (((length)+1)*3 > (size)*2)
#define WS_HEAP_PERTURB_SHIFT 5

void ws_heap_initialize(ws_heap *const result) {
    result->size = WS_HEAP_SIZE;
    result->length = 0;
    result->entries = (ws_heap_entry *)malloc(sizeof(ws_heap_entry) * WS_HEAP_SIZE);
    for(size_t i = 0; i < WS_HEAP_SIZE; i++) {
        result->entries[i].initialized = 0;
    }
}

void ws_heap_finish(ws_heap *const table) {
    for(size_t i = 0; i < table->size; i++) {
        if(table->entries[i].initialized) {
            ws_int_free(&table->entries[i].key);
            ws_int_free(&table->entries[i].value);
        }
    }
    free(table->entries);
}

void ws_heap_print(ws_heap *const table) {
    printf("hashtable size %#X, length %#X\n", table->size, table->length);
    char *keystr, *valstr;
    for(size_t i = 0; i < table->size; i++) {
        if (table->entries[i].initialized) {
            keystr = ws_int_to_dec_string(&table->entries[i].key);
            valstr = ws_int_to_dec_string(&table->entries[i].value);
            printf("%#4X %s: %s\n", i % table->size, keystr, valstr);
        }
    }
}

static size_t ws_heap_insert_position(const ws_heap *const table, const ws_int *const key) {
    //find the correct spot in the heap, this function is only for internal use
    unsigned int hash = ws_int_hash(key);
    unsigned int perturb = hash;
    size_t position = hash % table->size;
    while (table->entries[position].initialized &&
           ws_int_compare(&table->entries[position].key, key)) {
        position = (position * 5 + 1 + perturb) % table->size;
        perturb >>= WS_HEAP_PERTURB_SHIFT;
    }
    return position;
}

void ws_heap_set(ws_heap *const table, const ws_int *const key, const ws_int *const value) {
    // check if we'e getting too large, and resize
    size_t position;
    if (WS_HEAP_RESIZE_TIME(table->length, table->size)) {

        // oh boy, resizing. save the old table size and 
        ws_heap_entry *old = table->entries;
        size_t old_size = table->size;
        size_t old_length = table->length;

        // create a new array to hold everything
        table->size *= WS_HEAP_RESIZE_FACTOR;
        table->length = old_length;
        table->entries = (ws_heap_entry *)malloc(sizeof(ws_heap_entry) * table->size);

        // set everyting to uninitialized
        for(size_t i = 0; i < table->size; i++) {
            table->entries[i].initialized = 0;
        }

        // and populate the new array with the old values
        for(size_t i = 0; i < old_size; i++) {
            if (old[i].initialized) {
                position = ws_heap_insert_position(table, &old[i].key);
                table->entries[position].key = old[i].key;
                table->entries[position].value = old[i].value;
                table->entries[position].initialized = 1;
            }
        }

        // finally, free the old array
        free(old);
    }
    // actual insertion
    position = ws_heap_insert_position(table, key);

    if (!table->entries[position].initialized) {
        table->length++;
        table->entries[position].initialized = 1;
        table->entries[position].key = *key;
    } else {
        ws_int_free(key);
    }
    table->entries[position].value = *value;
}

void ws_heap_get(ws_int *const result, const ws_heap *const table, const ws_int *const key) {
    //find the spot key and return the value
    size_t position = ws_heap_insert_position(table, key);
    ws_int_free(key);
    if (table->entries[position].initialized) {
        *result = table->entries[position].value;
        return;
    }

    char *decstring = ws_int_to_dec_string(key);
    printf("Tried to look up value in the heap at %s which did not exist\n", decstring);
    free(decstring);
    exit(EXIT_FAILURE);
}



/* The stack, a simple resizing array
 */
#define WS_STACK_SIZE 1024
#define WS_STACK_RESIZE_FACTOR 2

void ws_stack_initialize(ws_stack *const result) {
    result->size = WS_STACK_SIZE;
    result->length = 0;
    result->entries = (ws_int *)malloc(sizeof(ws_int) * WS_STACK_SIZE);
}

void ws_stack_finish(ws_stack *const stack) {
    for(size_t i = 0; i < stack->length; i++) {
        ws_int_free(stack->entries + i);
    }
    free(stack->entries);
}

void ws_stack_print(ws_stack *const stack) {
    printf("stack contents with length %d:\n", stack->length);
    char *entstr;
    for (size_t i = 0; i < stack->length; i++) {
        entstr = ws_int_to_dec_string(stack->entries + i);
        printf("%s\n", entstr);
        free(entstr);
    }
}



/* Same for the callstack
 */
void ws_callstack_initialize(ws_callstack *const result) {
    result->size = WS_STACK_SIZE;
    result->length = 0;
    result->entries = (size_t *)malloc(sizeof(size_t) * WS_STACK_SIZE);
}

void ws_callstack_finish(ws_callstack *const stack) {
    free(stack->entries);
}



/* And implementations of all commands
 */
static void ws_command_push(ws_stack *const stack, const ws_int *const input) {
    if (stack->length == stack->size) {
        stack->size *= WS_STACK_RESIZE_FACTOR;
        stack->entries = (ws_int *)realloc(stack->entries, sizeof(ws_int) * stack->size);
    }

    ws_int_copy(stack->entries + stack->length++, input);
}

static void ws_command_duplicate(ws_stack *const stack) {

    ws_command_push(stack, stack->entries + stack->length-1);
}

static void ws_command_copy(ws_stack *const stack, const ws_int *const index) {
    int i = ws_int_to_int(index);
    if (i < 0 || i >= stack->length) {
        printf("Tried to copy from position not on stack\n");
        exit(EXIT_FAILURE);
    }
    ws_command_push(stack, stack->entries + i);
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

static void ws_command_discard(ws_int *const result, ws_stack *const stack) {
    if(!stack->length) {
        printf("tried to pop from empty stack\n");
        exit(EXIT_FAILURE);
    }
    if(result) {
        *result = stack->entries[--stack->length];
    } else {
        ws_int_free(stack->entries + (--stack->length));
    }
}

static void ws_command_slide(ws_stack *const stack, const ws_int *const amount) {
    ws_int tokeep = stack->entries[stack->length-1];
    int i = ws_int_to_int(amount);
    if (i < 0 || i >= stack->length) {
        printf("Tried to slide amount not on stack\n");
        exit(EXIT_FAILURE);
    }
    for (size_t pos = stack->length - 1 - i; pos < stack->length - 1; pos++) {
        ws_int_free(stack->entries + pos);
    }
    stack->length -= i;
    stack->entries[stack->length-1] = tokeep;
}

static void ws_command_add(ws_stack *const stack) {
    if(stack->length < 2) {
        printf("need at least two items on the stack to add\n");
        exit(EXIT_FAILURE);
    }
    ws_int temp;
    ws_int_add(&temp, stack->entries + stack->length - 2, stack->entries + stack->length - 1);
    ws_int_free(stack->entries + stack->length - 2);
    ws_int_free(stack->entries + stack->length - 1);
    stack->entries[stack->length - 2] = temp;
    stack->length--;
}

static void ws_command_subtract(ws_stack *const stack) {
    if(stack->length < 2) {
        printf("need at least two items on the stack to subtract\n");
        exit(EXIT_FAILURE);
    }
    ws_int temp;
    ws_int_subtract(&temp, stack->entries + stack->length - 2, stack->entries + stack->length - 1);
    ws_int_free(stack->entries + stack->length - 2);
    ws_int_free(stack->entries + stack->length - 1);
    stack->entries[stack->length - 2] = temp;
    stack->length--;
}

static void ws_command_multiply(ws_stack *const stack) {
    if(stack->length < 2) {
        printf("need at least two items on the stack to multiply\n");
        exit(EXIT_FAILURE);
    }
    ws_int temp;
    ws_int_multiply(&temp, stack->entries + stack->length - 2, stack->entries + stack->length - 1);
    ws_int_free(stack->entries + stack->length - 2);
    ws_int_free(stack->entries + stack->length - 1);
    stack->entries[stack->length - 2] = temp;
    stack->length--;
}

static void ws_command_divide(ws_stack *const stack) {
    if(stack->length < 2) {
        printf("need at least two items on the stack to divide\n");
        exit(EXIT_FAILURE);
    }
    ws_int temp;
    ws_int_divide(&temp, stack->entries + stack->length - 2, stack->entries + stack->length - 1);
    ws_int_free(stack->entries + stack->length - 2);
    ws_int_free(stack->entries + stack->length - 1);
    stack->entries[stack->length - 2] = temp;
    stack->length--;
}

static void ws_command_modulo(ws_stack *const stack) {
    if(stack->length < 2) {
        printf("need at least two items on the stack to modulo\n");
        exit(EXIT_FAILURE);
    }
    ws_int temp;
    ws_int_modulo(&temp, stack->entries + stack->length - 2, stack->entries + stack->length - 1);
    ws_int_free(stack->entries + stack->length - 2);
    ws_int_free(stack->entries + stack->length - 1);
    stack->entries[stack->length - 2] = temp;
    stack->length--;
}

static void ws_command_set(ws_stack *const stack, ws_heap *const heap) {
    ws_int value, key;
    ws_command_discard(&value, stack);
    ws_command_discard(&key, stack);
    ws_heap_set(heap, &key, &value); //don't have to free here since the heap consumes key and value
}

static void ws_command_get(ws_stack *const stack, ws_heap *const heap) {
    ws_int value, key;
    ws_command_discard(&key, stack);
    ws_heap_get(&value, heap, &key); //get consumes the key and does not return a copied ws_int, merely a reference
    ws_command_push(stack, &value);
}

static void ws_command_call(size_t *const next_index, ws_callstack *const callstack, const size_t dest) {
    if (callstack->length == callstack->size) {
        callstack->size *= WS_STACK_RESIZE_FACTOR;
        callstack->entries = (size_t *)realloc(callstack->entries, sizeof(size_t) * callstack->size);
    }
    callstack->entries[callstack->length++] = *next_index;
    *next_index = dest;
}

static void ws_command_jump(size_t *const next_index, const size_t dest) {
    *next_index = dest;
}

static void ws_command_jumpifzero(size_t *const next_index, ws_stack *const stack, const size_t dest) {
    ws_int test;
    ws_command_discard(&test, stack);
    if (ws_int_iszero(&test)) {
        *next_index = dest;
    }
    ws_int_free(&test);
}

static void ws_command_jumpifnegative(size_t *const next_index, ws_stack *const stack, const size_t dest) {
    ws_int test;
    ws_command_discard(&test, stack);
    if (ws_int_isnegative(&test)) {
        *next_index = dest;
    }
    ws_int_free(&test);
}

static void ws_command_endsubroutine(size_t *const next_index, ws_callstack *const callstack) {
    if (!callstack->length) {
        printf("can't end subroutine without calls on the callstack\n");
        exit(EXIT_FAILURE);
    }
    *next_index = callstack->entries[--(callstack->length)];
}

static void ws_command_endprogram(ws_callstack *const callstack) {
    if (callstack->length) {
        printf("warning: attempted to end the program with a non-empty callstack\n");
    }
}

static void ws_command_printchar(ws_stack *const stack) {
    ws_int test;
    ws_command_discard(&test, stack);

    putchar(ws_int_to_int(&test));

    ws_int_free(&test);
}

static void ws_command_printnum(ws_stack *const stack){
    ws_int test;
    ws_command_discard(&test, stack);

    char *buffer = ws_int_to_dec_string(&test);
    printf("%s", buffer);

    free(buffer);
    ws_int_free(&test);
}

static void ws_command_inputchar(ws_stack *const stack, ws_heap *const heap){
    int i = getchar();
    ws_int test, key;
    ws_int_from_int(&test, i, NULL);

    ws_int_copy(&key, stack->entries + stack->length - 1); //heap doesnt copy it
    ws_heap_set(heap, &key, &test); //heap consumes both, no need to free
}

static void ws_command_inputnum(ws_stack *const stack, ws_heap *const heap) {
    ws_int test, key;
    ws_int_input(&test);

    ws_int_copy(&key, stack->entries + stack->length - 1);
    ws_heap_set(heap, &key, &test);
}

#endif