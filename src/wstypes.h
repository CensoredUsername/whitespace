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
// in ws_label length represents the amount of bits, not amount of bytes.
typedef struct {
    char *data;
    size_t length;
} ws_string, ws_label;

// a whitespace command node. depending on the type and if it's parsed/compiled, the union contains:
// a: a big int, b: a string label, or c: an offset in the program
typedef struct {
    ws_command_type type; 
    union {
        ws_int parameter;
        ws_label label;
        size_t jumpoffset;
    };
#if DEBUG
    ws_string text;
#endif
} ws_command;

// a container for whitespace nodes. the types are purely for indicating wether the label compilation has been performed
typedef struct {
    int flags;
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
    ws_label key;
    size_t value;
    char initialized;
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

int ws_string_compare(const ws_string a, const ws_string b) {
    if (a.length != b.length) {
        return -1;
    }
    return memcmp(a.data, b.data, a.length);
}



/* Sadly we can't just treat labels as packed binary data, due to this forgetting about trailing 0's*/
#define ws_round8up(x) ((x)/8 + !!((x)%8) + (!x))
#define ws_hash_prime 1000003

ws_label ws_label_from_whitespace(const ws_string old) {
    //criteria for the return string. length is at least 1, or len(old)/8 rounded up
    size_t byte_length = ws_round8up(old.length);
    ws_label result = {(char *)malloc(byte_length), old.length};
    memset(result.data, 0, byte_length);
    
    for(size_t i = 0; i < old.length; i++) {
        result.data[byte_length - 1 - i/8] |= ((old.data[result.length - 1 - i] == SPACE)? 0: 1) << (i%8);
    }
    return result;
}

void ws_label_print(const ws_label input) {
    size_t length = ws_round8up(input.length);
    for(size_t i = 0; i < length; i++) {
        putchar(input.data[i]);
    }
}

void ws_label_free(const ws_label input) {
    free(input.data);
}

int ws_label_hash(const ws_label input) {
    // a very simple hashing function. it is primarily meant to be fast, collision resolution does the rest
    // similar to cpythons string hashing function.
    size_t length = ws_round8up(input.length);
    int value = input.data[0] << 7; // [0] always exists

    for(size_t i = 0; i < length; i++) {
        value = (value*ws_hash_prime) ^ input.data[i];
    }
    return value ^ input.length;
}

int ws_label_compare(const ws_label a, const ws_label b) {
    if (a.length != b.length) {
        return -1;
    }
    return memcmp(a.data, b.data, ws_round8up(a.length));
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

int ws_int_compare(const ws_int left, const ws_int right) {
    return left.data != right.data;
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


/* an implementation of a label: int map follows. 
 * It is implemented as a hash table
 * It is impossible to overwrite keys in this implementation, -1 will be returned.
 * for more comments see wsmachine's heap
 */
#define WS_MAP_SIZE 16
#define WS_MAP_RESIZE_FACTOR 4
#define WS_MAP_RESIZE_TIME(length, size) (((length)+1)*3 > (size)*2)
#define WS_MAP_PERTURB_SHIFT 5

static ws_map *ws_map_initialize() {
    ws_map *const map = (ws_map *)malloc(sizeof(ws_map));
    map->size = WS_MAP_SIZE;
    map->length = 0;
    map->entries = (ws_map_entry *)malloc(sizeof(ws_map_entry) * WS_MAP_SIZE);
    for(size_t i = 0; i < WS_MAP_SIZE; i++) {
        map->entries[i].initialized = 0;
    }
    return map;
}

static void ws_map_free(ws_map *const map) {
    for(size_t i = 0; i < map->size; i++) {
        if (map->entries[i].initialized) {
            ws_label_free(map->entries[i].key);
        }  
    }
    free(map->entries);
    free(map);
}

static void ws_map_print(ws_map *const map) {
    printf("hashtable size %#X, length %#X\n", map->size, map->length);
    for(size_t i = 0; i < map->size; i++) {
        if (map->entries[i].initialized) {
            printf("%#4X ", i % map->size);
            ws_label_print(map->entries[i].key);
            printf(": %4d\n", map->entries[i].value);
        } else {
            printf("%#4X NULL: NULL\n", i % map->size);
        }
    }
}

static int ws_map_insert(ws_map *const map, const ws_label key, const int value) {

    int hash = ws_label_hash(key);
    int perturb = hash;
    size_t position = hash % map->size;
    while (map->entries[position].initialized &&
           ws_label_compare(map->entries[position].key, key)) {
        position = (position * 5 + 1 + perturb) % map->size;
        perturb >>= WS_MAP_PERTURB_SHIFT;
    }
    if (map->entries[position].initialized) {
        return -1;
    }
    map->entries[position].initialized = 1;
    map->entries[position].key = key;
    map->entries[position].value = value;
    map->length++;
    return 0;
}

static int ws_map_set(ws_map *const map, const ws_label key, const size_t value) {
    // check if we should resize
    if (WS_MAP_RESIZE_TIME(map->length, map->size)) {

        ws_map_entry *old = map->entries;
        size_t old_size = map->size;

        map->size *= WS_MAP_RESIZE_FACTOR;
        map->length = 0;
        map->entries = (ws_map_entry *)malloc(sizeof(ws_map_entry) * map->size);

        for(size_t i = 0; i < map->size; i++) {
            map->entries[i].initialized = 0;
        }

        for(size_t i = 0; i < old_size; i++) {
            if (old[i].initialized) {
                ws_map_insert(map, old[i].key, old[i].value);
            }
        }

        free(old);
    }

    return ws_map_insert(map, key, value);
}

static int ws_map_get(const ws_map *const map, const ws_label key) {

    int hash = ws_label_hash(key);
    int perturb = hash;
    size_t position = hash % map->size;
    while (map->entries[position].initialized) {
        if (!ws_label_compare(map->entries[position].key, key)) {
            return map->entries[position].value;
        }
        position = (position * 5 + 1 + perturb) % map->size;
        perturb >>= WS_MAP_PERTURB_SHIFT;
    }
    return -1;
}
#endif