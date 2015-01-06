#include <boruvka/alloc.h>
#include "op_id_tr.h"

void planOpIdTrInit(plan_op_id_tr_t *tr, const plan_op_t *op, int op_size)
{
    int i;
    int max_glob_id = 0;

    tr->loc_to_glob_size = op_size;
    tr->loc_to_glob = BOR_ALLOC_ARR(int, tr->loc_to_glob_size);
    for (i = 0; i < op_size; ++i){
        tr->loc_to_glob[i] = op[i].global_id;
        if (op[i].global_id > max_glob_id)
            max_glob_id = op[i].global_id;
    }

    tr->glob_to_loc_size = max_glob_id + 1;
    tr->glob_to_loc = BOR_ALLOC_ARR(int, tr->glob_to_loc_size);
    for (i = 0; i < tr->glob_to_loc_size; ++i)
        tr->glob_to_loc[i] = -1;
    for (i = 0; i < op_size; ++i){
        tr->glob_to_loc[op[i].global_id] = i;
    }
}

void planOpIdTrFree(plan_op_id_tr_t *tr)
{
    if (tr->glob_to_loc)
        BOR_FREE(tr->glob_to_loc);
    if (tr->loc_to_glob)
        BOR_FREE(tr->loc_to_glob);
}
