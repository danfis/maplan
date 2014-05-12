#ifndef __FD_VAR_H__
#define __FD_VAR_H__

#include <stdlib.h>
#include <stdio.h>

struct _fd_var_t {
    char *name;
    int domain;
    char **fact_names;
    size_t fact_names_size;
    int axiom_layer;
    int default_axiom_value;
};
typedef struct _fd_var_t fd_var_t;

void fdVarInit(fd_var_t *fd);
void fdVarFree(fd_var_t *fd);
void fdVarLoadFromFile(fd_var_t *var, FILE *in);

#endif /* __FD_VAR_H__ */

