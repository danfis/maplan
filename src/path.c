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
        BOR_FREE(op);
    }
}

void planPathPrepend(plan_path_t *path, plan_operator_t *op)
{
    plan_path_op_t *path_op;

    path_op = BOR_ALLOC(plan_path_op_t);
    path_op->op = op;
    borListInit(&path_op->path);
    borListPrepend(path, &path_op->path);
}

void planPathPrint(const plan_path_t *path, FILE *fout)
{
    plan_path_op_t *op;

    BOR_LIST_FOR_EACH_ENTRY(path, plan_path_op_t, op, path){
        fprintf(fout, "(%s)\n", op->op->name);
    }
}

unsigned planPathCost(const plan_path_t *path)
{
    unsigned cost = 0;
    plan_path_op_t *op;

    BOR_LIST_FOR_EACH_ENTRY(path, plan_path_op_t, op, path){
        cost += op->op->cost;
    }

    return cost;
}
