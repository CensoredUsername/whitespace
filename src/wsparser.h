/* wsparser.h, parsers whitespace into an array of ws_command structs */
#ifndef WSPARSER_H
#define WSPARSER_H

#define PARAMETER_CACHE_SIZE 32
#define COMMANDLENGTH 24

#define COMMAND_ARRAY_SIZE 10
#define COMMAND_ARRAY_RESIZE 2

#include "wstypes.h"
#include "whitespace.h"

const ws_string ws_command_names[COMMANDLENGTH] = {
    {"push", 4},
    {"duplicate", 9},
    {"copy", 4},
    {"swap", 4},
    {"discard", 7},
    {"slide", 5},

    {"add", 3},
    {"subtract", 8},
    {"multiply", 8},
    {"divide", 6},
    {"modulo", 6},

    {"set", 3},
    {"get", 3},

    {"label", 5},
    {"call", 4},
    {"jump", 4},
    {"jumpifzero", 10},
    {"jumpifnegative", 14},
    {"endsubroutine", 13},
    {"endprogram", 10},

    {"printchar", 9},
    {"printnum", 8},
    {"inputchar", 9},
    {"inputnum", 8}
};

const ws_string ws_command_map[COMMANDLENGTH] = {
    {"  ", 2},
    {" \n ", 3},
    {" \t ", 3},
    {" \n\t", 3},
    {" \n\n", 3},
    {" \t\n", 3},

    {"\t   ", 4},
    {"\t  \t", 4},
    {"\t  \n", 4},
    {"\t \t ", 4},
    {"\t \t\t", 4},

    {"\t\t ", 3},
    {"\t\t\t", 3},

    {"\n  ", 3},
    {"\n \t", 3},
    {"\n \n", 3},
    {"\n\t ", 3},
    {"\n\t\t", 3},
    {"\n\t\n", 3},
    {"\n\n\n", 3},

    {"\t\n  ", 4},
    {"\t\n \t", 4},
    {"\t\n\t ", 4},
    {"\t\n\t\t", 4}
};

//data structures indicating if a certain command takes a parameter or a label
const char ws_parameter_map[COMMANDLENGTH] = {
    1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const char ws_label_map[COMMANDLENGTH] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0
};

void ws_visualize(const ws_string string) {
    for(size_t i = 0; i < string.length; i++) {
        switch (string.data[i]) {
            case SPACE:
                string.data[i] = 'S';
                break;
            case TAB:
                string.data[i] = 'T';
                break;
            case BREAK:
                string.data[i] = 'N';
                break;
        }
    }
}

static ws_parsed *ws_parsed_alloc(const size_t commandno) {
    ws_parsed *const result = (ws_parsed *)malloc(sizeof(ws_parsed));
    if (commandno) {
        result->commands = malloc(sizeof(ws_command) * commandno);
    }
    result->length = commandno;
    result->flags = 0;
    return result;
}

static void ws_parsed_free(ws_parsed *const program) {
    //free the program, commands, label strings and ws_ints
    ws_command *command;
    for(size_t i = 0; i < program->length; i++){
        command = program->commands + i;

        if (ws_parameter_map[command->type]) {
            ws_int_free(command->parameter);

        } else if (ws_label_map[command->type] && !(program->flags & 0x1)) {
            ws_label_free(command->label);
        }
#if DEBUG
        ws_string_free(command->text);
#endif
        free(command);
    }
    free(program);
}

ws_parsed *ws_parse(const ws_string text) {

    ws_command *command_array = (ws_command *)malloc(sizeof(ws_command)*COMMAND_ARRAY_SIZE);
    ws_command *current_node;
    size_t command_array_length = 0;
    size_t command_array_size = COMMAND_ARRAY_SIZE;

    char command_buffer[5] = {0};
    ws_string current_command = {command_buffer , 0};
    char succes;

    size_t parameter_start;
    size_t parameter_end;
    size_t parameter_max_size = PARAMETER_CACHE_SIZE;
    ws_string current_parameter = {(char *)malloc(PARAMETER_CACHE_SIZE), 0};
    size_t current_parameter_size;

#if DEBUG
    ws_string debug_text;
#endif

    size_t i = 0;
    while (i < text.length) {
        //this is the node to be set up
        current_node = command_array + command_array_length;

        //setup for command parsing, we'll loop one character at the time
        current_command.length = 0;
        succes = 0;
        while (!succes) {

            //check if we're not running out of bounds
            if (i == text.length) {
                printf("end of buffer while parsing command at position %d", i);
                exit(EXIT_FAILURE);
            }

            //there are no commands of length 5 or higher
            if (current_command.length==4) {
                printf("no valid command at position %d", i - 4);
                exit(EXIT_FAILURE);
            }

            //comments
            if (text.data[i] != SPACE && text.data[i] != TAB && text.data[i] != BREAK) {
                i++;
                continue;
            }

            current_command.data[current_command.length++] = text.data[i++];

            //check if we have a command, commands have at least length 2 and at most length 4
            if (current_command.length == 1) {
                continue;
            }

            for(size_t j = 0; j < COMMANDLENGTH; j++) {

                if (!ws_string_compare(ws_command_map[j], current_command)) {
                    current_node->type = j;
                    succes = 1;
                    break;
                }
            }
        }

        //parse parameter
        if (ws_parameter_map[current_node->type] || ws_label_map[current_node->type]) {
            parameter_start = i;

            //figure out the size of the parameter
            parameter_end = (size_t)((char *)memchr(text.data + parameter_start, BREAK, text.length - parameter_start) - text.data);
            if (!parameter_end) {
                printf("end of buffer while parsing parameter at position %d", parameter_start);
                exit(EXIT_FAILURE);
            }
            current_parameter_size = parameter_end - parameter_start + 1; //include the newline in here!

            //if our buffer isn't large enough, make it larger to fit
            if (parameter_max_size < current_parameter_size) {
                parameter_max_size = current_parameter_size;
                current_parameter.data = (char *)realloc(current_parameter.data, parameter_max_size);
            }

            //we know we can safely collect the characters of the parameter now
            current_parameter.length = 0;
            for(; i < (parameter_start + current_parameter_size); i++) {
                if (text.data[i] == SPACE || text.data[i] == TAB || text.data[i] == BREAK) {
                    current_parameter.data[current_parameter.length++] = text.data[i];
                }
            }

            current_parameter.length--; //newline

            //parse the parameters into their data structures
            if (ws_label_map[current_node->type]) {
                current_node->label = ws_label_from_whitespace(current_parameter);
            } else {
                current_node->parameter = ws_int_from_whitespace(current_parameter);
            }
#if DEBUG
            //if we're debugging, create new strings holding the whole whitespace code
            debug_text.length = current_command.length + current_parameter.length + 1;
            debug_text.data = (char *)malloc(debug_text.length);

            memcpy(debug_text.data, current_command.data, current_command.length);
            memcpy(debug_text.data + current_command.length, current_parameter.data , current_parameter.length + 1);
            ws_visualize(debug_text);
        } else {
            debug_text = ws_strcpy(current_command);
            ws_visualize(debug_text);
        }

        //these are added to the node and printed
        current_node->text = debug_text;
        printf("%s: ", ws_command_names[current_node->type]);
        ws_string_print(debug_text);
        putchar('\n');
#else
        }
#endif
        //bump command_array_length
        command_array_length++;

        //read till we encounter a new whitespace character
        while (i != text.length && (text.data[i] != SPACE && text.data[i] != TAB && text.data[i] != BREAK)) {
            i++;
            if (i == text.length) {
                break;
            }
        }
        
        //if we get here we're expecting a new whitespace command but we've maxed out our array
        if (command_array_size == command_array_length) {
            command_array_size *= COMMAND_ARRAY_RESIZE;
            command_array = (ws_command *)realloc(command_array, sizeof(ws_command)*command_array_size);
        }
    }

    //put the program together and clean up
    ws_string_free(current_parameter);

    if (!command_array_length) {
        printf("empty program\n");
        exit(EXIT_FAILURE);
    }

    ws_parsed *program = ws_parsed_alloc(0);
    program->commands = (ws_command *)realloc(command_array, sizeof(ws_command)*command_array_length);
    program->length = command_array_length;
    return program;
}

#endif
