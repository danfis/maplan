#ifndef __FD_VAR_H__
#define __FD_VAR_H__

#include <stdlib.h>
#include <stdio.h>

struct _fd_var_t {
    char *name;
    int range;
    char **fact_name;
    int axiom_layer;
};
typedef struct _fd_var_t fd_var_t;

void fdVarInit(fd_var_t *fd);
void fdVarFree(fd_var_t *fd);

#endif /* __FD_VAR_H__ */

