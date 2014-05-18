#include <boruvka/alloc.h>
#include "fd/var.h"

void fdVarInit(fd_var_t *fd)
{
    fd->name = NULL;
    fd->range = -1;
    fd->fact_name = NULL;
    fd->axiom_layer = -1;
}

void fdVarFree(fd_var_t *fd)
{
    int i;

    if (fd->name)
        BOR_FREE(fd->name);

    if (fd->fact_name != NULL){
        for (i = 0; i < fd->range; ++i){
            BOR_FREE(fd->fact_name[i]);
        }
        BOR_FREE(fd->fact_name);
    }
}

