#ifndef __FD_FD_H__
#define __FD_FD_H__

#include <fd/var.h>

struct _fd_t {
    fd_var_t *vars;
    size_t vars_size;
};
typedef struct _fd_t fd_t;


/**
 * Creates a new (empty) instance of a Fast Downward algorithm.
 */
fd_t *fdNew(void);

/**
 * Deletes fd instance.
 */
void fdDel(fd_t *fd);

/**
 * Loads definitions from specified file.
 */
void fdLoadFromFile(fd_t *fd, const char *filename);

#endif /* __FD_FD_H__ */
