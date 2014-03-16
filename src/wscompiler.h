#ifndef WSCOMPILER_H
#define WSCOMPILER_H
/* the main task of compiling is to replace the goto labels by indexes of the actual label */

#include "wstypes.h"
#include "wsparser.h"

#define WS_JUMPOFFSETS_SIZE 32
#define WS_JUMPOFFSETS_RESIZE 2
/* compiling is done in two steps.
 * first, iterate through all nodes and store all known labels and their offsets
 * second, iterate through all nodes and for all jumps, substitute the label through the actual offset
 * the strings will be free'd in the process, this is a lossy process, label names are lost in this step
 */
ws_compiled *ws_compile(ws_parsed *const parsed) {
    if (parsed->flags & 0x1) {
        printf("cannot compile already compiled program");
        exit(EXIT_FAILURE);
    }
    size_t jump_offsets_size = WS_JUMPOFFSETS_SIZE;
    size_t jump_offsets_length = 0;
    size_t *jump_offsets = (size_t *)malloc(sizeof(size_t) * WS_JUMPOFFSETS_SIZE);

    int offset;
    ws_command *current_command;

    ws_map *const map = ws_map_initialize();

    for(size_t i = 0; i < parsed->length; i++) {
        current_command = parsed->commands + i;

        if (current_command->type == label) {

            // insert the label offset into the hashmap
            if (ws_map_set(map, current_command->label, i)) {

                printf("duplicate label found at command %d\n", i);
                exit(EXIT_FAILURE);
            }
            current_command->jumpoffset = i;

        } else if (ws_label_map[current_command->type]) {

            // resize if necessary
            if (jump_offsets_size == jump_offsets_length) {
                jump_offsets_size *= 2;
                jump_offsets = (size_t *)realloc(jump_offsets, sizeof(size_t) * jump_offsets_size);
            }

            jump_offsets[jump_offsets_length++] = i;

        }
    }
    
    //for each of the jump/calls, replace the label by 
    for(size_t i = 0; i < jump_offsets_length; i++) {
        current_command = parsed->commands + jump_offsets[i];
        offset = ws_map_get(map, current_command->label);

        if (offset < 0) {
            printf("label not found at command %d\n", jump_offsets[i]);
            exit(EXIT_FAILURE);
        }
        current_command->jumpoffset = offset;
    }

    ws_map_free(map); //note, this frees all the old label strings too because I didn't copy the char *'s.
    free(jump_offsets);

    parsed->flags |= 0x1;
    return (ws_compiled *)parsed;
}

#endif