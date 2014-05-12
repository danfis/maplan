#include <string.h>

#include <boruvka/alloc.h>

#include "fd/fd.h"

#define READ_BUFSIZE 1024

fd_t *fdNew(void)
{
    fd_t *fd;

    fd = BOR_ALLOC(fd_t);
    fd->vars = NULL;
    fd->vars_size = 0;

    return fd;
}

void fdDel(fd_t *fd)
{
    size_t i;

    if (fd->vars != NULL){
        for (i = 0; i < fd->vars_size; ++i){
            fdVarFree(fd->vars + i);
        }
        BOR_FREE(fd->vars);
    }
}


static int readCheckWord(FILE *in, const char *word)
{
    char in_word[READ_BUFSIZE];

    if (fscanf(in, "%s", in_word) != 1){
        // TODO
        fprintf(stderr, "Error: Expecting: %s and nothing was read\n", word);
        return 0;
    }

    if (strcmp(in_word, word) != 0){
        fprintf(stderr, "Error: Expecting: %s, getting %s instead\n", word, in_word);
        return 0;
    }

    return 1;
}

static int readVersion(FILE *in)
{
    int version;

    readCheckWord(in, "begin_version");
    if (fscanf(in, "%d", &version) != 1){
        fprintf(stderr, "Error: expectiong version.\n");
        return -1;
    }
    readCheckWord(in, "end_version");

    return version;
}

void fdLoadFromFile(fd_t *fd, const char *filename)
{
    FILE *fin;

    fin = fopen(filename, "r");
    if (fin == NULL){
        fprintf(stderr, "Could not open file %s\n", filename);
        exit(-1);
    }

    // Ignore version, just read it
    readVersion(fin);

    fclose(fin);
}
