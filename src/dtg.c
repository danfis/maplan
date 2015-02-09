#include <boruvka/alloc.h>

#include "plan/dtg.h"

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

void planDTGInit(plan_dtg_t *dtg, const plan_var_t *var, int var_size,
                 const plan_op_t *op, int op_size)
{
    int i, size;

    // Allocate structures for all DTGs
    dtg->var_size = var_size;
    dtg->dtg = BOR_ALLOC_ARR(plan_dtg_var_t, var_size);
    for (i = 0; i < var_size; ++i){
        dtg->dtg[i].var = i;
        dtg->dtg[i].val_size = var[i].range;
        size = var[i].range * var[i].range;
        dtg->dtg[i].trans = BOR_CALLOC_ARR(plan_dtg_trans_t, size);
    }

    for (i = 0; i < op_size; ++i){
        dtgUpdateByOp(dtg, op + i);
    }
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

static void dtgVarPrint(const plan_dtg_var_t *dtg, FILE *fout)
{
    plan_var_id_t var;
    plan_val_t val;
    int i, j, k, vi;
    const plan_dtg_trans_t *trans;

    for (i = 0; i < dtg->val_size; ++i){
        for (j = 0; j < dtg->val_size; ++j){
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
