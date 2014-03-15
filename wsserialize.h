#ifndef WSSERIALIZE_H
#define WSSERIALIZE_H

#define SERIALIZING_BUFFER_SIZE 1000
#define SERIALIZING_BUFFER_RESIZE 2

#include "wstypes.h"

static ws_serializing_buffer serializing_buffer() {
    ws_serializing_buffer dest;
    dest.length = SERIALIZING_BUFFER_SIZE;
    dest.buffer = (char *)malloc(SERIALIZING_BUFFER_SIZE);
    dest.index = 0;
    return dest;
}

static ws_string serializing_finish(const ws_serializing_buffer dest) {
    ws_string result = {(char *)realloc(dest.buffer, dest.index), dest.index};
    return result;
}

static void serializing_free(const ws_serializing_buffer dest) {
    free(dest.buffer);
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

static void serialize_uchar(const unsigned char number, ws_serializing_buffer *const dest) {
#if (DEBUG)
    printf("serializing uchar\n");
#endif
    reserve_space(dest, 1);
    memcpy(dest->buffer+dest->index, &number, 1);
    dest->index++;
}

static unsigned char unserialize_uchar(ws_serializing_buffer *const source) {
#if (DEBUG)
    printf("unserializing uchar\n");
#endif
    unsigned char data;
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

static void serialize_string(const ws_string string, ws_serializing_buffer *const dest) {
#if (DEBUG)
    printf("serializing string\n");
#endif
    serialize_uint32(string.length, dest);

    reserve_space(dest, string.length);
    memcpy(dest->buffer+dest->index, string.data, string.length);
    dest->index += string.length;
}

static ws_string unserialize_string(ws_serializing_buffer *const source) {
#if (DEBUG)
    printf("unserializing string\n");
#endif
    ws_string string;

    string.length = unserialize_uint32(source);

    check_space(source, string.length);
    string.data = source->buffer + source->index;

    source->index += string.length;
    return ws_strcpy(string);
}

static void serialize_ws_int(const ws_int number, ws_serializing_buffer *const dest) {
    serialize_int32(ws_int_to_int(number), dest);
}

static ws_int unserialize_ws_int(ws_serializing_buffer *const source) {
    return ws_int_from_int(unserialize_int32(source));
}

static void serialize_command(const ws_command *const command, const int compiled, ws_serializing_buffer *const dest) {
    serialize_uint32(command->type, dest);
    if (ws_parameter_map[command->type]) {
        serialize_ws_int(command->parameter, dest);
    } else if (ws_label_map[command->type]) {
        if (compiled) {
            serialize_uint32(command->jumpoffset, dest);
        } else {
            serialize_string(command->label, dest);
        }
    }
}

static void unserialize_command(ws_command *const command, const int compiled, ws_serializing_buffer *const source) {
    command->type = unserialize_uint32(source);
    if (ws_parameter_map[command->type]) {
        command->parameter = unserialize_ws_int(source);
    } else if (ws_label_map[command->type]) {
        if (compiled) {
            command->jumpoffset = unserialize_uint32(source);
        } else {
            command->label = unserialize_string(source);
        }
    }
}

static void serialize_program(const ws_parsed *const program, ws_serializing_buffer *const dest) {
    serialize_int32(program->compiled, dest);
    serialize_uint32(program->length, dest);
    for (size_t i = 0; i < (program->length); i++) {
        serialize_command(program->commands + i, program->compiled, dest);
    }
}

static ws_parsed *unserialize_program(ws_serializing_buffer *const source) {
    const int compiled = unserialize_int32(source);
    const size_t length = unserialize_uint32(source);
    ws_parsed *const program = ws_parsed_alloc(length);
    program->compiled = compiled;
    for(size_t i = 0; i < program->length; i++) {
        unserialize_command(program->commands + i, compiled, source);
    }
    return program;
}

ws_string ws_serialize(const ws_parsed *const program) {
    ws_serializing_buffer dest = serializing_buffer(NULL);
    serialize_program(program, &dest);
    return serializing_finish(dest);
}

ws_parsed *ws_unserialize(const ws_string buffer) {
    ws_serializing_buffer source = {buffer.length, buffer.data, 0};
    return unserialize_program(&source);
}

#endif