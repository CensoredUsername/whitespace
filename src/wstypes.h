/* wstypes.h - definition of general use types */
#ifndef WSTYPES_H
#define WSTYPES_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define DEBUG 0

#define SPACE ' '
#define TAB '\t'
#define BREAK '\n'

#define ws_hash_prime 1000003

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

// a container of a char pointer and size_t length for easy manipulation of strings
// in ws_label length represents the amount of bits, not amount of bytes.
typedef struct {
    char *data;
    size_t length;
} ws_string, ws_label;

//this needs the definition of ws_string
#include "wsint.h"

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
} ws_program;



/* Helper functions for dealining with the ws_string struct. 
 * This struct is mostly just syntactic sugar for a char arran and size_t length
 */
void ws_string_fromchar(ws_string *const output, char *const buffer, const size_t length){
    output->data = buffer;
    output->length = length;
}

void ws_strcpy(ws_string *const result, const ws_string *const old) {
    char *const buffer = malloc(old->length);
    memcpy(buffer, old->data, old->length);
    result->data = buffer;
    result->length = old->length;
}

void ws_string_free(const ws_string *const input) {
    free(input->data);
}

void ws_string_print(const ws_string *const text) {
    for(size_t i = 0; i < text->length; i++) {
        putchar(text->data[i]);
    }
}

unsigned int ws_string_hash(const ws_string *const input) {
    /* a very simple hashing function. it is primarily meant to be fast, collision resolution does the rest
     * similar to cpythons string hashing function.
     */
    if (!input->length) {
        return 0;
    }
    unsigned int value = input->data[0] << 7;
    for(size_t i = 0; i < input->length; i++) {
        value = (value * ws_hash_prime) ^ input->data[i];
    }
    return value ^ input->length;
}

int ws_string_compare(const ws_string *const a, const ws_string *const b) {
    if (a->length != b->length) {
        return -1;
    }
    return memcmp(a->data, b->data, a->length);
}



/* Labels are very similar to strings, 
 * except that we store the length of the bits in the label
 * not the bytes, due to that being lossy on trailing 0's
 */
#define ws_round8up(x) ((x)/8 + !!((x)%8) + (!x))

void ws_label_from_whitespace(ws_label *const result, const ws_string *const old) {
    //criteria for the return string. length is at least 1, or len(old)/8 rounded up
    size_t byte_length = ws_round8up(old->length);
    result->length = old->length;
    result->data = (char *)malloc(byte_length);
    memset(result->data, 0, byte_length);
    
    for(size_t i = 0; i < old->length; i++) {
        result->data[byte_length - 1 - i/8] |= ((old->data[result->length - 1 - i] == SPACE)? 0: 1) << (i%8);
    }
}

void ws_label_print(const ws_label *const input) {
    size_t length = ws_round8up(input->length);
    for(size_t i = 0; i < length; i++) {
        putchar(input->data[i]);
    }
}

void ws_label_free(const ws_label *const input) {
    free(input->data);
}

unsigned int ws_label_hash(const ws_label *const input) {
    /* An adapted version of the string hashing function
     */
    size_t length = ws_round8up(input->length);
    unsigned int value = input->data[0] << 7; // [0] always exists

    for(size_t i = 0; i < length; i++) {
        value = (value*ws_hash_prime) ^ input->data[i];
    }
    return value ^ input->length;
}

int ws_label_compare(const ws_label *const a, const ws_label *const b) {
    if (a->length != b->length) {
        return -1;
    }
    return memcmp(a->data, b->data, ws_round8up(a->length));
}

#endif