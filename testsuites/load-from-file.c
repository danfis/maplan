#include <cu/cu.h>
#include "fd/fd.h"


TEST(testLoadFromFile)
{
    fd_t *fd;

    fd = fdNew();
    fdLoadFromJsonFile(fd, "load-from-file.in1.json");
    fdDel(fd);
}
