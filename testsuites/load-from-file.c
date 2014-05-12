#include <cu/cu.h>
#include "fd/fd.h"


TEST(testLoadFromFile)
{
    fd_t *fd;

    fd = fdNew();
    fdLoadFromFile(fd, "load-from-file.in1.txt");
    fdDel(fd);
}
