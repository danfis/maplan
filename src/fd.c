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

static int readInt(FILE *in, const char *suffix)
{
    char token[READ_BUFSIZE];
    int value;

    sprintf(token, "begin_%s", suffix);
    readCheckWord(in, token);
    if (fscanf(in, "%d", &value) != 1){
        fprintf(stderr, "Error: expectiong %s.\n", suffix);
        return -1;
    }
    sprintf(token, "end_%s", suffix);
    readCheckWord(in, token);

    // TODO
    fprintf(stderr, "Value %s: %d\n", suffix, value);

    return value;
}

static int readVersion(FILE *in)
{
    return readInt(in, "version");
}

static int readMetric(FILE *in)
{
    return readInt(in, "metric");
}

static void loadVars(FILE *in, fd_t *fd)
{
    int count;
    size_t i, j, tmp;
    char str[READ_BUFSIZE];
    fd_var_t *var;

    if (fscanf(in, "%d", &count) != 1){
        // TODO
    }

    fd->vars_size = count;
    fd->vars = BOR_ALLOC_ARR(fd_var_t, fd->vars_size);
    for (i = 0; i < fd->vars_size; i++){
        fdVarInit(fd->vars + i);
    }

    for (i = 0; i < fd->vars_size; i++){
        var = fd->vars + i;

        readCheckWord(in, "begin_variable");

        if (fscanf(in, "%s", str) != 1){
            fprintf(stderr, "XXX\n");
            // TODO
        }
        // TODO
        var->name = strdup(str);

        if (fscanf(in, "%d", &var->axiom_layer) != 1){
            // TODO
            fprintf(stderr, "Error: Could not load axiom layer\n");
        }

        if (fscanf(in, "%d", &var->domain) != 1){
            // TODO
            fprintf(stderr, "Error: Could not domain range\n");
        }

        var->fact_names_size = var->domain;
        var->fact_names = BOR_ALLOC_ARR(char *, var->fact_names_size);
        for (j = 0; j < var->fact_names_size; ++j){
            var->fact_names[j] = NULL;
        }

        // read the rest of the line
        tmp = 0;
        getline(&var->fact_names[0], &tmp, in);
        for (j = 0; j < var->fact_names_size; ++j){
            // TODO
            tmp = 0;
            getline(&var->fact_names[j], &tmp, in);
        }

        readCheckWord(in, "end_variable");
    }

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
    // TODO
    readMetric(fin);

    loadVars(fin, fd);

    fclose(fin);
}
