#ifndef WSSERIALIZE_H
#define WSSERIALIZE_H

#define SERIALIZING_BUFFER_SIZE 1000
#define SERIALIZING_BUFFER_RESIZE 2

#include "wstypes.h"



//the buffer used when serializing
typedef struct {
    size_t length;
    char *buffer;
    size_t index;
} ws_serializing_buffer;



/* First a set serializing and unserializing functiosn for each datatype
 * Is provided, and finally the functions to serialize and unserialize the whole
 * thing
 */
static void serializing_buffer_initialize(ws_serializing_buffer *const dest) {
    dest->length = SERIALIZING_BUFFER_SIZE;
    dest->buffer = (char *)malloc(SERIALIZING_BUFFER_SIZE);
    dest->index = 0;
}

static void serializing_buffer_finish(ws_string *const result, const ws_serializing_buffer *const dest) {
    result->data = (char *)realloc(dest->buffer, dest->index);
    result->length = dest->index;
}

static void reserve_space(ws_serializing_buffer *const dest, const size_t size) {
    while ((dest->index + size) > (dest->length)) {
        dest->length *= 2;
        dest->buffer = (char *)realloc(dest->buffer, dest->length);
        if (!dest->buffer) {
            printf("out of memory in reserve_space\n");
            exit(EXIT_FAILURE);
        }
    }
}

static void check_space(const ws_serializing_buffer *const source, const size_t size) {
    if (size > (source->length - source->index)) {
        printf("tried to read out of source buffer bounds\n");
        exit(EXIT_FAILURE);
    }
}

static void serialize_char(const char number, ws_serializing_buffer *const dest) {
#if (DEBUG)
    printf("serializing uchar\n");
#endif
    reserve_space(dest, 1);
    memcpy(dest->buffer+dest->index, &number, 1);
    dest->index++;
}

static unsigned char unserialize_char(ws_serializing_buffer *const source) {
#if (DEBUG)
    printf("unserializing uchar\n");
#endif
    char data;
    check_space(source, 1);
    memcpy(&data, source->buffer + source->index, 1);
    source->index++;
    return data;
}

static void serialize_uint32(const uint32_t number, ws_serializing_buffer *const dest) {
#if (DEBUG)
    printf("serializing uint32\n");
#endif
    reserve_space(dest, sizeof(uint32_t));
    memcpy(dest->buffer+dest->index, &number, sizeof(uint32_t));
    dest->index += sizeof(uint32_t);
}

static uint32_t unserialize_uint32(ws_serializing_buffer *const source) {
#if (DEBUG)
    printf("unserializing uint32\n");
#endif
    size_t data;
    check_space(source, sizeof(uint32_t));
    memcpy(&data, source->buffer + source->index, sizeof(uint32_t));
    source->index += sizeof(uint32_t);
    return data;
}

void serialize_int32(const int32_t number, ws_serializing_buffer *const dest) {
#if (DEBUG)
    printf("serializing int32\n");
#endif
    reserve_space(dest, sizeof(int32_t));
    memcpy(dest->buffer+dest->index, &number, sizeof(int32_t));
    dest->index += sizeof(int32_t);
}

static int32_t unserialize_int32(ws_serializing_buffer *const source) {
#if (DEBUG)
    printf("unserializing int32\n");
#endif
    int data;
    check_space(source, sizeof(int32_t));
    memcpy(&data, source->buffer + source->index, sizeof(int32_t));
    source->index += sizeof(int32_t);
    return data;

}

static void serialize_label(const ws_label *const string, ws_serializing_buffer *const dest) {
#if (DEBUG)
    printf("serializing label\n");
#endif
    serialize_uint32(string->length, dest);
    size_t length = ws_round8up(string->length);

    reserve_space(dest, length);
    memcpy(dest->buffer+dest->index, string->data, length);
    dest->index += length;
}

static void unserialize_label(ws_label *const string, ws_serializing_buffer *const source) {
#if (DEBUG)
    printf("unserializing label\n");
#endif
    string->length = unserialize_uint32(source);
    size_t length = ws_round8up(string->length);
    string->data = (char *)malloc(length);

    check_space(source, length);
    memcpy(string->data, source->buffer + source->index, length);
    source->index += string->length;
}

static void serialize_ws_int(const ws_int *const number, ws_serializing_buffer *const dest) {
    serialize_uint32(number->length, dest);
    size_t length = number->length;
    if (number->length) {
        for (size_t i = 0; i < length; i++) {
            serialize_uint32(number->digits[i], dest);
        }
    } else {
        serialize_int32(number->data, dest);
    }
}

static void unserialize_ws_int(ws_int *const result, ws_serializing_buffer *const source) {
    result->length = unserialize_uint32(source);
    size_t length = ACTLEN(result->length);
    result->digits = (digit *)malloc(sizeof(digit) * length);
    if (result->length) {
        for (size_t i = 0; i < length; i++) {
            result->digits[i] = unserialize_uint32(source);
        }
    } else {
        result->data = unserialize_int32(source);
    }
}

static void serialize_command(const ws_command *const command, const int compiled, ws_serializing_buffer *const dest) {
#if DEBUG
    serialize_char(0xff, dest);
#endif
    serialize_char(command->type, dest);
    if (ws_parameter_map[command->type]) {
        serialize_ws_int(&command->parameter, dest);
    } else if (ws_label_map[command->type]) {
        if (compiled) {
            serialize_uint32(command->jumpoffset, dest);
        } else {
            serialize_label(&command->label, dest);
        }
    }
}

static void unserialize_command(ws_command *const command, const int compiled, ws_serializing_buffer *const source) {
#if DEBUG
    if (unserialize_char(source) != 0xff) {
        printf("expected new command while serializing");
        exit(EXIT_FAILURE);
    }
#endif
    command->type = unserialize_char(source);
    if (ws_parameter_map[command->type]) {
        unserialize_ws_int(&command->parameter, source);
    } else if (ws_label_map[command->type]) {
        if (compiled) {
            command->jumpoffset = unserialize_uint32(source);
        } else {
            unserialize_label(&command->label, source);
        }
    }
}

static void serialize_program(const ws_program *const program, ws_serializing_buffer *const dest) {
    serialize_int32(program->flags, dest);
    serialize_uint32(program->length, dest);
    for (size_t i = 0; i < (program->length); i++) {
        serialize_command(program->commands + i, program->flags & 0x1, dest);
    }
}

static void unserialize_program(ws_program *const program, ws_serializing_buffer *const source) {
    const int flags = unserialize_int32(source);
    const size_t length = unserialize_uint32(source);
    ws_program_initialize(program, length);
    program->flags = flags;
    for(size_t i = 0; i < program->length; i++) {
        unserialize_command(program->commands + i, flags & 0x1, source);
    }
}




void ws_serialize(ws_string *buffer, const ws_program *const program) {
    ws_serializing_buffer dest;
    serializing_buffer_initialize(&dest);
    serialize_program(program, &dest);
    serializing_buffer_finish(buffer, &dest);
}

void ws_unserialize(ws_program *const program, const ws_string *const buffer) {
    ws_serializing_buffer source = {buffer->length, buffer->data, 0};
    unserialize_program(program, &source);
}

#endif