/* whitespace.c */

#include "whitespace.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("expected at least one argument\n");
        exit(EXIT_FAILURE);
    }

    FILE *wsfile = fopen(argv[1], "rb");

    if (!wsfile) {
        printf("failure to open file\n");
        exit(EXIT_FAILURE);
    }

    fseek(wsfile, 0L, SEEK_END);
    int wssize = ftell(wsfile);
    rewind(wsfile);

    if (wssize < 0) {
        printf("file doesn't have size\n");
        exit(EXIT_FAILURE);
    }

    ws_string data = {(char *)malloc(wssize), 0};
    if (!data.data) {
        printf("couldn't allocate buffer for file");
    }

    data.length = fread(data.data, 1, wssize, wsfile);
    fclose(wsfile);

    ws_parsed *a = ws_parse(data);
    ws_string_free(data);

    size_t length =strlen(argv[1]);
    char *compiledname = (char *)malloc(length+2);
    memcpy(compiledname, argv[1], length);
    memcpy(compiledname + length, "c\0", 2);

    ws_string serialized = ws_serialize(a);
    FILE *wscfile = fopen(compiledname, "wb");
    fwrite(serialized.data, serialized.length, 1, wscfile);
    fclose(wscfile);
    ws_string_free(serialized);

    ws_compiled *b = ws_compile(a);

    ws_execute(b);
    ws_parsed_free(b);

    return 0;
}