/* wstypes.h - definition of general use types */
#ifndef WSTYPES_H
#define WSTYPES_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

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

// placeholder struct for bigint
typedef struct {
    size_t length;
    union {
        int data;
        char *digits;
    };
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
} ws_program;



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

unsigned int ws_string_hash(const ws_string input) {
    /* a very simple hashing function. it is primarily meant to be fast, collision resolution does the rest
     * similar to cpythons string hashing function.
     */
    if (!input.length) {
        return 0;
    }
    unsigned int value = input.data[0] << 7;
    for(size_t i = 0; i < input.length; i++) {
        value = (value * ws_hash_prime) ^ input.data[i];
    }
    return value ^ input.length;
}

int ws_string_compare(const ws_string a, const ws_string b) {
    if (a.length != b.length) {
        return -1;
    }
    return memcmp(a.data, b.data, a.length);
}



/* Labels are very similar to strings, 
 * except that we store the length of the bits in the label
 * not the bytes, due to that being lossy on trailing 0's
 */
#define ws_round8up(x) ((x)/8 + !!((x)%8) + (!x))

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

unsigned int ws_label_hash(const ws_label input) {
    /* An adapted version of the string hashing function
     */
    size_t length = ws_round8up(input.length);
    unsigned int value = input.data[0] << 7; // [0] always exists

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
 * It's still a work in progress, anyway the idea is
 * If length is zero, then data is just an int. This is mainly for speed reasons
 * Else, digits will be an array of something holding the value
 */
void ws_int_free(const ws_int input) {
    //current implementation of ws_int doesn't allocate anything dynamically
    if(input.length) {
        free(input.digits);
    }
}

ws_int ws_int_copy(const ws_int input) {
    if (input.length) {
        printf("length: %d ", input.length);
        printf("bigint not yet supported in copy\n");
    }
    return input;
}

ws_int ws_int_from_int(const int input) {
    ws_int result;
    result.length = 0;
    result.data = input;
    return result;
}

ws_int ws_int_from_whitespace(const ws_string string) {
    
    //protect against empty parameters or sign only parameters
    if(string.length < 2) {
        return ws_int_from_int(0);
    }

    int accumulator = 0;
    size_t datalength = string.length-1;
    // note that since whitespace uses ones complement while we use twos complement, all 32 long whiespace 
    // parameters are valid ints
    if (datalength > 31) { 
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

    return ws_int_from_int(accumulator);
}

int ws_int_to_int(const ws_int input) {
    if (input.length) {
        printf("length: %d ", input.length);
        printf("bigint not yet supported\n");
    }
    return input.data;
}

void ws_int_print(const ws_int input) {
    if (input.length) {
        printf("length: %d ", input.length);
        printf("bigint not yet supported\n");
    }
    printf("%d", input.data);
}

ws_int ws_int_input() {
    int input;
    scanf("%d", &input);
    return ws_int_from_int(input);
}

unsigned int ws_int_hash(const ws_int input) {
    if (input.length) {
        printf("length: %d ", input.length);
        printf("bigint not yet supported\n");
    }
    return (unsigned int)input.data;
}

int ws_int_compare(const ws_int left, const ws_int right) {
    if (left.length || right.length) {
        printf("length: %d %d ", left.length, right.length);
        printf("bigint not yet supported\n");
    }
    return left.data != right.data;
}

ws_int ws_int_multiply(const ws_int left, const ws_int right) {
    if (left.length || right.length) {
        printf("length: %d %d ", left.length, right.length);
        printf("bigint not yet supported\n");
    }
    ws_int result = {0, {left.data * right.data}};
    return result;
}

ws_int ws_int_divide(const ws_int left, const ws_int right) {
    if (left.length || right.length) {
        printf("length: %d %d ", left.length, right.length);
        printf("bigint not yet supported\n");
    }
    ws_int result = {0, {left.data / right.data}};
    return result;
}

ws_int ws_int_modulo(const ws_int left, const ws_int right) {
    if (left.length || right.length) {
        printf("length: %d %d ", left.length, right.length);
        printf("bigint not yet supported\n");
    }
    ws_int result = {0, {left.data % right.data}};
    return result;
}

ws_int ws_int_add(const ws_int left, const ws_int right) {
    if (left.length || right.length) {
        printf("length: %d %d ", left.length, right.length);
        printf("bigint not yet supported\n");
    }
    ws_int result = {0, {left.data + right.data}};
    return result;
}

ws_int ws_int_subtract(const ws_int left, const ws_int right) {
    if (left.length || right.length) {
        printf("length: %d %d ", left.length, right.length);
        printf("bigint not yet supported\n");
    }
    ws_int result = {0, {left.data - right.data}};
    return result;
}

int ws_int_iszero(const ws_int input) {
    if (input.length) {
        printf("length: %d ", input.length);
        printf("bigint not yet supported\n");
    }
    return input.data == 0;
}

int ws_int_isnegative(const ws_int input) {
    if (input.length) {
        printf("length: %d ", input.length);
        printf("bigint not yet supported\n");
    }
    return input.data < 0;
}

#endif