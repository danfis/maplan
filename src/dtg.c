/***
 * maplan
 * -------
 * Copyright (c)2015 Daniel Fiser <danfis@danfis.cz>,
 * Agent Technology Center, Department of Computer Science,
 * Faculty of Electrical Engineering, Czech Technical University in Prague.
 * All rights reserved.
 *
 * This file is part of maplan.
 *
 * Distributed under the OSI-approved BSD License (the "License");
 * see accompanying file BDS-LICENSE for details or see
 * <http://www.opensource.org/licenses/bsd-license.php>.
 *
 * This software is distributed WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the License for more information.
 */

#include <boruvka/alloc.h>
#include <boruvka/fifo.h>

#include "plan/dtg.h"

void planDTGPathInit(plan_dtg_path_t *path)
{
    path->length = 0;
    path->trans = NULL;
}

void planDTGPathFree(plan_dtg_path_t *path)
{
    if (path->trans)
        BOR_FREE(path->trans);
}

static void dtgConnect(plan_dtg_var_t *dtg, plan_val_t pre, plan_val_t eff,
                       const plan_op_t *op)
{
    plan_dtg_trans_t *trans;

    trans = dtg->trans + (pre * dtg->val_size) + eff;
    ++trans->ops_size;
    trans->ops = BOR_REALLOC_ARR(trans->ops, const plan_op_t *,
                                 trans->ops_size);
    trans->ops[trans->ops_size - 1] = op;
}

static void dtgConnectAll(plan_dtg_var_t *dtg, plan_val_t eff,
                          const plan_op_t *op)
{
    plan_val_t pre;

    for (pre = 0; pre < dtg->val_size; ++pre){
        if (pre != eff)
            dtgConnect(dtg, pre, eff, op);
    }
}

static void dtgUpdateByOp(plan_dtg_t *dtg, const plan_op_t *op)
{
    plan_var_id_t var;
    plan_val_t pre, eff;
    int tmp;

    PLAN_PART_STATE_FOR_EACH(op->eff, tmp, var, eff){
        if (planPartStateIsSet(op->pre, var)){
            pre = planPartStateGet(op->pre, var);
            dtgConnect(dtg->dtg + var, pre, eff, op);
        }else{
            dtgConnectAll(dtg->dtg + var, eff, op);
        }
    }
}

static void _planDTGInit(plan_dtg_t *dtg, const plan_var_t *var, int var_size,
                         const plan_op_t *op, int op_size, int range_add)
{
    int i, size;

    // Allocate structures for all DTGs
    dtg->var_size = var_size;
    dtg->dtg = BOR_ALLOC_ARR(plan_dtg_var_t, var_size);
    for (i = 0; i < var_size; ++i){
        if (var[i].ma_privacy){
            dtg->dtg[i].var = i;
            dtg->dtg[i].val_size = 0;
            dtg->dtg[i].trans = NULL;
        }else{
            dtg->dtg[i].var = i;
            dtg->dtg[i].val_size = var[i].range + range_add;
            size = dtg->dtg[i].val_size * dtg->dtg[i].val_size;
            dtg->dtg[i].trans = BOR_CALLOC_ARR(plan_dtg_trans_t, size);
        }
    }

    for (i = 0; i < op_size; ++i){
        dtgUpdateByOp(dtg, op + i);
    }
}

void planDTGInit(plan_dtg_t *dtg, const plan_var_t *var, int var_size,
                 const plan_op_t *op, int op_size)
{
    _planDTGInit(dtg, var, var_size, op, op_size, 0);
}

void planDTGInitUnknown(plan_dtg_t *dtg, const plan_var_t *var, int var_size,
                        const plan_op_t *op, int op_size)
{
    _planDTGInit(dtg, var, var_size, op, op_size, 1);
}

void planDTGFree(plan_dtg_t *dtg)
{
    int i, j, size;

    for (i = 0; i < dtg->var_size; ++i){
        if (dtg->dtg[i].trans){
            size = dtg->dtg[i].val_size;
            size *= size;
            for (j = 0; j < size; ++j){
                if (dtg->dtg[i].trans[j].ops)
                    BOR_FREE(dtg->dtg[i].trans[j].ops);
            }
            BOR_FREE(dtg->dtg[i].trans);
        }
    }
    BOR_FREE(dtg->dtg);
}

void planDTGAddTrans(plan_dtg_t *dtg, plan_var_id_t var, plan_val_t from,
                     plan_val_t to, const plan_op_t *op)
{
    dtgConnect(dtg->dtg + var, from, to, op);
}

int planDTGHasTrans(const plan_dtg_t *dtg, plan_var_id_t var,
                    plan_val_t from, plan_val_t to,
                    const plan_op_t *op)
{
    plan_dtg_trans_t *trans;
    int i;

    if (dtg->dtg[var].trans == NULL)
        return 0;

    trans = dtg->dtg[var].trans + (from * dtg->dtg[var].val_size) + to;
    for (i = 0; i < trans->ops_size; ++i){
        if (trans->ops[i] == op)
            return 1;
    }

    return 0;
}

int planDTGPath(const plan_dtg_t *_dtg, plan_var_id_t var,
                plan_val_t from, plan_val_t to,
                plan_dtg_path_t *path)
{
    const plan_dtg_var_t *dtg = _dtg->dtg + var;
    const plan_dtg_trans_t *trans;
    bor_fifo_t fifo;
    int i, *pre, val, found = 0;

    if (from == to || dtg->trans == NULL)
        return 0;

    // Initialize array for predecessors
    pre = BOR_ALLOC_ARR(int, dtg->val_size);
    for (i = 0; i < dtg->val_size; ++i)
        pre[i] = -1;

    borFifoInit(&fifo, sizeof(int));

    // Use BFS to search graph
    pre[from] = from;
    val = from;
    borFifoPush(&fifo, &val);
    while (!borFifoEmpty(&fifo) && !found){
        val = *(int *)borFifoFront(&fifo);
        borFifoPop(&fifo);

        trans = dtg->trans + (val * dtg->val_size);
        for (i = 0; i < dtg->val_size; ++i, ++trans){
            if (i == val || pre[i] != -1 || trans->ops_size == 0)
                continue;
            pre[i] = val;
            if (i == to){
                found = 1;
                break;
            }
            borFifoPush(&fifo, &i);
        }
    }

    if (found){
        // Determine length of the path
        path->length = 0;

        val = to;
        while (pre[val] != val){
            ++path->length;
            val = pre[val];
        }

        // Read out path
        path->trans = BOR_ALLOC_ARR(const plan_dtg_trans_t *, path->length);
        val = to;
        for (i = path->length - 1; i >= 0; --i){
            path->trans[i] = dtg->trans + (pre[val] * dtg->val_size) + val;
            val = pre[val];
        }
    }

    borFifoFree(&fifo);
    BOR_FREE(pre);

    if (found)
        return 0;
    return -1;
}


static void dtgVarPrint(const plan_dtg_var_t *dtg, FILE *fout)
{
    plan_var_id_t var;
    plan_val_t val;
    int i, j, k, vi;
    const plan_dtg_trans_t *trans;

    for (i = 0; i < dtg->val_size; ++i){
        for (j = 0; j < dtg->val_size; ++j){
            if (dtg->trans == NULL)
                continue;

            trans = dtg->trans + (i * dtg->val_size) + j;
            for (k = 0; k < trans->ops_size; ++k){
                fprintf(fout, "  (%d) -- ", i);
                fprintf(fout, "[%d]%s", trans->ops[k]->global_id,
                        trans->ops[k]->name);
                fprintf(fout, " {");
                PLAN_PART_STATE_FOR_EACH(trans->ops[k]->pre, vi, var, val){
                    if (vi != 0)
                        fprintf(fout, ",");
                    fprintf(fout, "%d:%d", (int)var, (int)val);
                }
                fprintf(fout, "}->{");
                PLAN_PART_STATE_FOR_EACH(trans->ops[k]->eff, vi, var, val){
                    if (vi != 0)
                        fprintf(fout, ",");
                    fprintf(fout, "%d:%d", (int)var, (int)val);
                }
                fprintf(fout, "}");
                fprintf(fout, " --> (%d)\n", j);
            }
        }
    }
}

void planDTGPrint(const plan_dtg_t *dtg, FILE *fout)
{
    int i;

    for (i = 0; i < dtg->var_size; ++i){
        fprintf(fout, "DTG %d (range: %d):\n", i, dtg->dtg[i].val_size);
        dtgVarPrint(dtg->dtg + i, fout);
    }
}
