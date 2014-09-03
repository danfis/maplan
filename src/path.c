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
}

void planPathCopy(plan_path_t *dst, const plan_path_t *src)
{
    plan_path_op_t *op;
    const plan_path_op_t *src_op;

    borListInit(dst);
    BOR_LIST_FOR_EACH_ENTRY(src, plan_path_op_t, src_op, path){
        op = BOR_ALLOC(plan_path_op_t);
        memcpy(op, src_op, sizeof(plan_path_op_t));
        op->name = strdup(src_op->name);
        borListAppend(dst, &op->path);
    }
}

void planPathPrepend(plan_path_t *path, plan_op_t *op,
                     plan_state_id_t from, plan_state_id_t to)
{
    plan_path_op_t *path_op;

    path_op = BOR_ALLOC(plan_path_op_t);
    path_op->name = strdup(op->name);
    path_op->cost = op->cost;
    path_op->op = op;
    path_op->from_state = from;
    path_op->to_state = to;
    borListInit(&path_op->path);
    borListPrepend(path, &path_op->path);
}

void planPathPrepend2(plan_path_t *path, const char *op_name,
                      plan_cost_t op_cost)
{
    plan_path_op_t *path_op;

    path_op = BOR_ALLOC(plan_path_op_t);
    path_op->name = strdup(op_name);
    path_op->cost = op_cost;
    path_op->op = NULL;
    path_op->from_state = PLAN_NO_STATE;
    path_op->to_state = PLAN_NO_STATE;
    borListInit(&path_op->path);
    borListPrepend(path, &path_op->path);
}

void planPathPrint(const plan_path_t *path, FILE *fout)
{
    plan_path_op_t *op;

    BOR_LIST_FOR_EACH_ENTRY(path, plan_path_op_t, op, path){
        fprintf(fout, "(%s)\n", op->name);
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
