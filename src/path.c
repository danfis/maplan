#include <boruvka/alloc.h>
#include "plan/path.h"

void planPathInit(plan_path_t *path)
{
    borListInit(path);
}

void planPathFree(plan_path_t *path)
{
    bor_list_t *item;
    plan_path_op_t *op;

    while (!borListEmpty(path)){
        item = borListNext(path);
        borListDel(item);
        op = BOR_LIST_ENTRY(item, plan_path_op_t, path);
        if (op->name)
            BOR_FREE(op->name);
        BOR_FREE(op);
    }

    borListInit(path);
}

void planPathCopy(plan_path_t *dst, const plan_path_t *src)
{
    plan_path_op_t *op;
    const plan_path_op_t *src_op;

    borListInit(dst);
    BOR_LIST_FOR_EACH_ENTRY(src, plan_path_op_t, src_op, path){
        op = BOR_ALLOC(plan_path_op_t);
        memcpy(op, src_op, sizeof(plan_path_op_t));
        op->name = BOR_STRDUP(src_op->name);
        borListAppend(dst, &op->path);
    }
}

void planPathCopyFactored(plan_path_t *dst, const plan_path_t *src,
                          int agent_id)
{
    plan_path_op_t *op;
    const plan_path_op_t *src_op;
    int timestamp = 1;

    borListInit(dst);
    BOR_LIST_FOR_EACH_ENTRY(src, plan_path_op_t, src_op, path){
        if (src_op->owner == agent_id){
            op = BOR_ALLOC(plan_path_op_t);
            memcpy(op, src_op, sizeof(plan_path_op_t));
            op->name = BOR_STRDUP(src_op->name);
            op->timestamp = timestamp;
            borListAppend(dst, &op->path);
        }

        ++timestamp;
    }
}

void planPathPrepend(plan_path_t *path, const char *name,
                     plan_cost_t cost, int global_id, int owner,
                     plan_state_id_t from, plan_state_id_t to)
{
    plan_path_op_t *path_op;

    path_op = BOR_ALLOC(plan_path_op_t);
    path_op->name = BOR_STRDUP(name);
    path_op->cost = cost;
    path_op->global_id = global_id;
    path_op->owner = owner;
    path_op->from_state = from;
    path_op->to_state = to;
    path_op->timestamp = -1;
    borListInit(&path_op->path);
    borListPrepend(path, &path_op->path);
}

void planPathPrependOp(plan_path_t *path, plan_op_t *op,
                       plan_state_id_t from, plan_state_id_t to)
{
    planPathPrepend(path, op->name, op->cost, op->global_id, op->owner,
                    from, to);
}

void planPathPrepend2(plan_path_t *path, const char *op_name,
                      plan_cost_t op_cost)
{
    plan_path_op_t *path_op;

    path_op = BOR_ALLOC(plan_path_op_t);
    path_op->name = BOR_STRDUP(op_name);
    path_op->cost = op_cost;
    path_op->global_id = -1;
    path_op->owner = -1;
    path_op->from_state = PLAN_NO_STATE;
    path_op->to_state = PLAN_NO_STATE;
    path_op->timestamp = -1;
    borListInit(&path_op->path);
    borListPrepend(path, &path_op->path);
}

void planPathPrint(const plan_path_t *path, FILE *fout)
{
    plan_path_op_t *op;

    BOR_LIST_FOR_EACH_ENTRY(path, plan_path_op_t, op, path){
        if (op->timestamp > 0){
            fprintf(fout, "%d: (%s)\n", op->timestamp, op->name);
        }else{
            fprintf(fout, "(%s)\n", op->name);
        }
    }
}

plan_cost_t planPathCost(const plan_path_t *path)
{
    plan_cost_t cost = PLAN_COST_ZERO;
    plan_path_op_t *op;

    BOR_LIST_FOR_EACH_ENTRY(path, plan_path_op_t, op, path){
        cost += op->cost;
    }

    return cost;
}

int planPathLen(const plan_path_t *path)
{
    plan_path_op_t *op;
    int len = 0;

    BOR_LIST_FOR_EACH_ENTRY(path, plan_path_op_t, op, path)
        ++len;
    return len;
}
