#include <boruvka/alloc.h>

#include "_problem.h"

void planProblemFactIdInit(plan_problem_fact_id_t *factid,
                           const plan_var_t *var, int var_size)
{
    int i, j, id;

    // allocate the array corresponding to the variables
    factid->fact_id = BOR_ALLOC_ARR(int *, var_size);

    for (id = 0, i = 0; i < var_size; ++i){
        // allocate array for variable's values
        factid->fact_id[i] = BOR_ALLOC_ARR(int, var[i].range);

        // fill values with IDs
        for (j = 0; j < var[i].range; ++j)
            factid->fact_id[i][j] = id++;
    }

    // remember number of values
    factid->fact_size = id;
    factid->var_size = var_size;
}

void planProblemFactIdFree(plan_problem_fact_id_t *factid)
{
    int i;
    for (i = 0; i < factid->fact_size; ++i){
        if (factid->fact_id[i])
            BOR_FREE(factid->fact_id[i]);
    }
    BOR_FREE(factid->fact_id);
}
