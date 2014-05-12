#include <boruvka/alloc.h>
#include "fd/var.h"

    char *name;
    int domain;
    char **fact_names;
    size_t fact_names_size;
    int axiom_layer;
    int default_axiom_value;
void fdVarInit(fd_var_t *fd)
{
    fd->name = NULL;
    fd->domain = -1;
    fd->fact_names = NULL;
    fd->fact_names_size = 0;
    fd->axiom_layer = -1;
    fd->default_axiom_value = -1;
}

void fdVarFree(fd_var_t *fd)
{
    size_t i;

    if (fd->name)
        BOR_FREE(fd->name);

    if (fd->fact_names != NULL){
        for (i = 0; i < fd->fact_names_size; ++i){
            BOR_FREE(fd->fact_names[i]);
        }
        BOR_FREE(fd->fact_names);
    }
}

void fdVarLoadFromFile(fd_var_t *var, FILE *in)
{
}
