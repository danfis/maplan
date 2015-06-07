#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <plan/msg_schema.h>
#include <boruvka/alloc.h>

#include "msg-schema.h"


int main(int argc, char *argv[])
{
    msg_t msg;
    int fsize, pos;
    uint32_t size;
    char *data;
    FILE *fin;

    if (argc != 2){
        fprintf(stderr, "Usage: %s in.bin >out.txt\n", argv[0]);
        return -1;
    }

    fin = fopen(argv[1], "rb");
    if (fin == NULL){
        fprintf(stderr, "Error: Could not open `%s'\n", argv[1]);
        return -1;
    }

    fseek(fin, 0, SEEK_END);
    fsize = ftell(fin);
    fseek(fin, 0, SEEK_SET);
    data = malloc(fsize + 1);
    fread(data, fsize, 1, fin);
    fclose(fin);

    pos = 0;
    while (pos < fsize){
        size = *(uint32_t *)data;
        size = le32toh(size);

        data += 4;
        pos += 4;

        bzero(&msg, sizeof(msg));
        planMsgDecode(&msg, &schema_main, data);
        wr(&msg, stdout);
        data += size;
        pos += size;
    }

    return 0;
}
