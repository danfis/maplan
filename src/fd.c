#include <string.h>
#include <jansson.h>

#include <boruvka/alloc.h>

#include "fd/fd.h"

#define READ_BUFSIZE 1024

fd_t *fdNew(void)
{
    fd_t *fd;

    fd = BOR_ALLOC(fd_t);
    fd->var = NULL;
    fd->var_size = 0;

    return fd;
}

void fdDel(fd_t *fd)
{
    size_t i;

    if (fd->var != NULL){
        for (i = 0; i < fd->var_size; ++i){
            fdVarFree(fd->var + i);
        }
        BOR_FREE(fd->var);
    }

    BOR_FREE(fd);
}

static void loadVariable(fd_var_t *var, json_t *json)
{
    json_t *data, *jval;
    int i;

    data = json_object_get(json, "name");
    var->name = strdup(json_string_value(data));

    data = json_object_get(json, "range");
    var->range = json_integer_value(data);

    data = json_object_get(json, "layer");
    var->axiom_layer = json_integer_value(data);

    var->fact_name = BOR_ALLOC_ARR(char *, var->range);
    data = json_object_get(json, "fact_name");
    for (i = 0; i < var->range; i++){
        if (i < (int)json_array_size(data)){
            jval = json_array_get(data, i);
            var->fact_name[i] = strdup(json_string_value(jval));
        }else{
            var->fact_name[i] = strdup("");
        }
    }

#if 0
    fprintf(stderr, "%s: %d | %d\n", var->name, var->range, var->axiom_layer);
    for (i = 0; i < var->range; ++i){
        fprintf(stderr, " %s", var->fact_name[i]);
    }
    fprintf(stderr, "\n");
#endif
}

static void loadVariables(fd_t *fd, json_t *json)
{
    size_t i;
    json_t *json_var;

    // allocate array for variables
    fd->var_size = json_array_size(json);
    fd->var = BOR_ALLOC_ARR(fd_var_t, fd->var_size);
    for (i = 0; i < fd->var_size; ++i){
        fdVarInit(fd->var + i);
    }

    json_array_foreach(json, i, json_var){
        loadVariable(fd->var + i, json_var);
    }
}

void fdLoadFromJsonFile(fd_t *fd, const char *filename)
{
    json_t *json;
    json_t *data;
    json_error_t json_err;

    // TODO: Clear fd_t structure

    // open JSON data from file
    json = json_load_file(filename, 0, &json_err);
    if (json == NULL){
        // TODO: Error logging
        fprintf(stderr, "Error: Could not read json data. Line %d: %s.\n",
                json_err.line, json_err.text);
        exit(-1);
    }

    data = json_object_get(json, "variable");
    if (data == NULL){
        fprintf(stderr, "Error: No 'variable' key in json definition.\n");
        exit(-1);
    }
    loadVariables(fd, data);


    // free json resources
    json_decref(json);

#if 0

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
#endif
}
