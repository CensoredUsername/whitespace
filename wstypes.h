/* wstypes.h - definition of general use types */
#ifndef WSTYPES_H
#define WSTYPES_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

//the constants used to indicate the type of ws command the current node has
typedef enum {
    push            = 0,
    duplicate       = 1,
    copy            = 2,
    swap            = 3,
    discard         = 4,
    slide           = 5,

    add             = 6,
    subtract        = 7,
    multiply        = 8,
    divide          = 9,
    modulo          = 10,

    set             = 11,
    get             = 12,

    label           = 13,
    call            = 14,
    jump            = 15,
    jumpifzero      = 16,
    jumpifnegative  = 17,
    endsubroutine   = 18,
    endprogram      = 19,

    printchar       = 20,
    printnum        = 21,
    inputchar       = 22,
    inputnum        = 23
} ws_command_type;

// placeholder struct for bigint
typedef struct {
    int data;
} ws_int;

// a container of a char pointer and size_t length for easy manipulation of strings
typedef struct {
    char *data;
    size_t length;
} ws_string;

// a whitespace command node. depending on the type and if it's parsed/compiled, the union contains:
// a: a big int, b: a string label, or c: an offset in the program
typedef struct {
    ws_command_type type; 
    union {
        ws_int parameter;
        ws_string label;
        size_t jumpoffset;
    };
#if DEBUG
    ws_string text;
#endif
} ws_command;

// a container for whitespace nodes. the types are purely for indicating wether the label compilation has been performed
typedef struct {
    int compiled;
    size_t length;
    ws_command *commands;
} ws_parsed, ws_compiled;

typedef struct {
    size_t length;
    char *buffer;
    size_t index;
} ws_serializing_buffer;

// a map entry used for compiling labels
typedef struct {
    ws_string label;
//    uint32_t hash;
    int offset;
} ws_map_entry;

// the map used for compiling labels
typedef struct {
    ws_map_entry *entries;
    size_t size;
    size_t length;
} ws_map;

/* Helper functions for dealining with the ws_string struct. 
 * This struct is mostly just syntactic sugar for a char arran and size_t length
 * Just copy them around, they're tiny anyway.
 */
ws_string ws_string_fromchar(char *const buffer, const size_t length){
    const ws_string string = {buffer, length};
    return string;
}

ws_string ws_strcpy(const ws_string old) {
    char *const buffer = malloc(old.length);
    memcpy(buffer, old.data, old.length);
    ws_string result = {buffer, old.length};
    return result;
}

void ws_string_free(const ws_string old) {
    free(old.data);
}

void ws_string_print(const ws_string text) {
    for(size_t i = 0; i < text.length; i++) {
        putchar(text.data[i]);
    }
}

int ws_strcmp(const ws_string a, const ws_string b) {
    if (a.length != b.length) {
        return -1;
    }
    return memcmp(a.data, b.data, a.length);
}

ws_string ws_string_from_whitespace(const ws_string old) {
    //criteria for the return string. length is at least 1, or len(old)/8 rounded up
    ws_string result;
    result.length = 1 + old.length / 8 - (old.length % 8 == 0 && old.length != 0);
    result.data = (char *)malloc(result.length);
    memset(result.data, 0, result.length);
    for(size_t i = 0; i < old.length; i++) {
        result.data[i/8] |= ((old.data[i] == SPACE)? 0: 1) << (7 - (i%8));
    }
    return result;
}


/* Placeholder implementation of a bigint.
 * currently it's just a normal int in a structure, but
 * this is easily altered.
 */
void ws_int_free(const ws_int input) {
    //current implementation of ws_int doesn't allocate anything dynamically
}

ws_int ws_int_from_whitespace(const ws_string string) {
    ws_int result;
    if (string.length < 2) {
        result.data = 0;
        return result;
    }

    int accumulator = 0;
    size_t datalength = string.length-1;
    if (datalength >= 32) {
        printf("overflow in creating ws_int");
        exit(EXIT_FAILURE);
    }
    if (string.data[0] == SPACE) { //positive
        for(size_t i = datalength; i > 0; i--) {
            accumulator += (string.data[i] == TAB) << (datalength-i);
        }
    } else {
        for(size_t i = datalength; i > 0; i--) {
            accumulator -= (string.data[i] == TAB) << (datalength-i);
        }
    }
    result.data = accumulator;
    return result;
}

ws_int ws_int_from_int(const int input) {
    const ws_int result = {input};
    return result;
}

int ws_int_to_int(const ws_int input) {
    return input.data;
}

void ws_int_print(const ws_int input) {
    printf("%d", input.data);
}

ws_int ws_int_input() {
    int input;
    scanf("%d", &input);
    return ws_int_from_int(input);
}

int ws_int_cmp(const ws_int left, const ws_int right) {
    return left.data == right.data;
}

int ws_int_hash(const ws_int input) {
    return input.data;
}

ws_int ws_int_multiply(const ws_int left, const ws_int right) {
    ws_int result = {left.data * right.data};
    return result;
}

ws_int ws_int_divide(const ws_int left, const ws_int right) {
    ws_int result = {left.data / right.data};
    return result;
}

ws_int ws_int_modulo(const ws_int left, const ws_int right) {
    ws_int result = {left.data % right.data};
    return result;
}

ws_int ws_int_add(const ws_int left, const ws_int right) {
    ws_int result = {left.data + right.data};
    return result;
}

ws_int ws_int_subtract(const ws_int left, const ws_int right) {
    ws_int result = {left.data - right.data};
    return result;
}


/* an implementation of a string: int map follows. 
 * This implementation is rather crude, and has O(n) lookup time.
 # To be replaced by a better one, probably similar to the hashtable which the machine uses
 */
#define WS_MAP_SIZE 32
#define WS_MAP_RESIZE 2

static ws_map *ws_map_alloc() {
    ws_map *const map = (ws_map *)malloc(sizeof(ws_map));
    map->size = WS_MAP_SIZE;
    map->length = 0;
    map->entries = (ws_map_entry *)malloc(sizeof(ws_map_entry) * WS_MAP_SIZE);
    return map;
}

static void ws_map_checksize(ws_map *const map) {
    if (map->length == map->size) {
        map->size *= WS_MAP_RESIZE;
        map->entries = (ws_map_entry *)realloc(map->entries, sizeof(ws_map_entry) * map->size);
    }
}

static void ws_map_free(ws_map *const map) {
    for(size_t i = 0; i < map->length; i++) {
        ws_string_free(map->entries[i].label);
    }
    free(map->entries);
    free(map);
}

static int ws_map_insert(ws_map *const map, const ws_string label, const int offset) {
    ws_map_entry *current_entry;
    for(size_t i = 0; i < map->length; i++) {
        current_entry = map->entries + i;
        if (!ws_strcmp(current_entry->label, label)) {
            return -1;
        }
    }
    ws_map_checksize(map);
    map->entries[map->length].label = label;
    map->entries[map->length].offset = offset;
    map->length++;
    return 0;
}

static int ws_map_get(const ws_map *const map, const ws_string label) {
    ws_map_entry *current_entry;
    for(size_t i = 0; i < map->length; i++) {
        current_entry = map->entries + i;
        if (!ws_strcmp(current_entry->label, label)) {
            return current_entry->offset;
        }
    }
    return -1;
}
#endif