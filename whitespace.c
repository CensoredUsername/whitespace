/* whitespace.c */

#include "whitespace.h"
#include <stdio.h>

int main(int argc, char **argv) {
    char program[] = "   \t\n|\n   \t    \t\t\n| \n |\t\n \t|   \t \t \n|\t\n  |   \t\n|\t   | \n |   \t \t\t\n|\t  \t|\n\t  \t   \t \t\n|\n \n \t    \t\t\n|\n   \t   \t \t\n| \n\n|\n\n\n||||||";
    ws_parsed *test = ws_parse(ws_string_fromchar(program, strlen(program)));
    ws_compiled *test2 = ws_compile(test);
    ws_execute(test2);
    ws_parsed_free(test2);
    return 0;
}
