#include <boruvka/alloc.h>

#include "fact_id.h"

void planFactIdInit(plan_fact_id_t *fid, const plan_var_t *var, int var_size)
{
    int i, j, id;

    // allocate the array corresponding to the variables
    fid->fact_id = BOR_ALLOC_ARR(int *, var_size);

    for (id = 0, i = 0; i < var_size; ++i){
        // allocate array for variable's values
        fid->fact_id[i] = BOR_ALLOC_ARR(int, var[i].range);

        // fill values with IDs
        for (j = 0; j < var[i].range; ++j)
            fid->fact_id[i][j] = id++;
    }

    // remember number of values
    fid->fact_size = id;
    fid->var_size = var_size;
}

void planFactIdFree(plan_fact_id_t *fid)
{
    int i;
    for (i = 0; i < fid->var_size; ++i)
        BOR_FREE(fid->fact_id[i]);
    BOR_FREE(fid->fact_id);
}
