#ifndef WSCOMPILER_H
#define WSCOMPILER_H
/* the main task of compiling is to replace the goto labels by indexes of the actual label */

#include "wstypes.h"

#define WS_JUMPOFFSETS_SIZE 32
#define WS_JUMPOFFSETS_RESIZE 2



/* compiling is done in two steps.
 * first, iterate over the entire program, storing all indexes which have labels, and adding the index
 * of labels to a map.
 * second, iterate over the indexes which have labels, and replace the labels by the actual offsets in the
 * program.
 * the labels will be free'd in the process.
 */

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

/* forward declarations
 */
static void ws_map_initialize(ws_map *);
static int ws_map_set(ws_map *, const ws_label *, const size_t);
static int ws_map_get(const ws_map *, const ws_label *);
static void ws_map_finish(ws_map *);



/* As can be seen, compiling just alters the parsed struct
 */
void ws_compile(ws_program *const parsed) {
    if (parsed->flags & 0x1) {
        printf("cannot compile already compiled program");
        exit(EXIT_FAILURE);
    }
    size_t jump_offsets_size = WS_JUMPOFFSETS_SIZE;
    size_t jump_offsets_length = 0;
    size_t *jump_offsets = (size_t *)malloc(sizeof(size_t) * WS_JUMPOFFSETS_SIZE);

    int offset;
    ws_command *current_command;

    ws_map map;
    ws_map_initialize(&map);

    for(size_t i = 0; i < parsed->length; i++) {
        current_command = parsed->commands + i;

        if (current_command->type == label) {

            // insert the label offset into the hashmap
            if (ws_map_set(&map, &current_command->label, i)) {

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
        offset = ws_map_get(&map, &current_command->label);

        if (offset < 0) {
            printf("label not found at command %d\n", jump_offsets[i]);
            exit(EXIT_FAILURE);
        }
        current_command->jumpoffset = offset;
    }

    ws_map_finish(&map); //note, this frees all the old label strings too because I didn't copy the char *'s.
    free(jump_offsets);

    parsed->flags |= 0x1;
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

static void ws_map_initialize(ws_map *const map) {
    map->size = WS_MAP_SIZE;
    map->length = 0;
    map->entries = (ws_map_entry *)malloc(sizeof(ws_map_entry) * WS_MAP_SIZE);
    for(size_t i = 0; i < WS_MAP_SIZE; i++) {
        map->entries[i].initialized = 0;
    }
}

static void ws_map_finish(ws_map *const map) {
    for(size_t i = 0; i < map->size; i++) {
        if (map->entries[i].initialized) {
            ws_label_free(&map->entries[i].key);
        }  
    }
    free(map->entries);
}

static void ws_map_print(ws_map *const map) {
    printf("hashtable size %#X, length %#X\n", map->size, map->length);
    for(size_t i = 0; i < map->size; i++) {
        if (map->entries[i].initialized) {
            printf("%#4X ", i % map->size);
            ws_label_print(&map->entries[i].key);
            printf(": %4d\n", map->entries[i].value);
        } else {
            printf("%#4X NULL: NULL\n", i % map->size);
        }
    }
}

static int ws_map_insert(ws_map *const map, const ws_label *const key, const int value) {

    unsigned int hash = ws_label_hash(key);
    unsigned int perturb = hash;
    size_t position = hash % map->size;
    while (map->entries[position].initialized &&
           ws_label_compare(&map->entries[position].key, key)) {
        position = (position * 5 + 1 + perturb) % map->size;
        perturb >>= WS_MAP_PERTURB_SHIFT;
    }
    if (map->entries[position].initialized) {
        return -1;
    }
    map->entries[position].initialized = 1;
    map->entries[position].key = *key; //not ws_string_copying here since he char * should be freed
    map->entries[position].value = value;
    map->length++;
    return 0;
}

static int ws_map_set(ws_map *const map, const ws_label *const key, const size_t value) {
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
                ws_map_insert(map, &old[i].key, old[i].value);
            }
        }

        free(old);
    }

    return ws_map_insert(map, key, value);
}

static int ws_map_get(const ws_map *const map, const ws_label *const key) {

    unsigned int hash = ws_label_hash(key);
    unsigned int perturb = hash;
    size_t position = hash % map->size;
    while (map->entries[position].initialized) {
        if (!ws_label_compare(&map->entries[position].key, key)) {
            return map->entries[position].value;
        }
        position = (position * 5 + 1 + perturb) % map->size;
        perturb >>= WS_MAP_PERTURB_SHIFT;
    }
    return -1;
}

#endif