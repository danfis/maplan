#include <boruvka/alloc.h>
#include <plan/problem.h>
#include <plan/pddl.h>
#include <plan/pddl_ground.h>
#include <plan/pddl_sas.h>

static void formatFactName(const plan_pddl_sas_t *sas,
                           const plan_pddl_ground_t *ground,
                           const plan_pddl_t *pddl,
                           int fact_id, char *dst)
{
    const plan_pddl_fact_t *fact;
    const plan_pddl_predicate_t *pred;
    plan_pddl_fact_pool_t *fact_pool;
    int i, len;

    fact_pool = (plan_pddl_fact_pool_t *)&ground->fact_pool;
    fact = planPDDLFactPoolGet(fact_pool, fact_id);
    pred = pddl->predicate.pred + fact->pred;

    len = sprintf(dst, "%s(", pred->name);
    dst += len;

    for (i = 0; i < fact->arg_size; ++i){
        if (i > 0){
            len = sprintf(dst, ", ");
            dst += len;
        }

        len = sprintf(dst, "%s", pddl->obj.obj[fact->arg[i]].name);
        dst += len;
    }

    sprintf(dst, ")");
}

static int createVar(plan_problem_t *p, const plan_pddl_sas_t *sas,
                     const plan_pddl_ground_t *ground, const plan_pddl_t *pddl)
{
    char name[1024];
    plan_var_t *var;
    int i;

    p->var = BOR_ALLOC_ARR(plan_var_t, sas->var_size);
    p->var_size = sas->var_size;

    for (i = 0; i < sas->var_size; ++i){
        sprintf(name, "var%d", i);
        planVarInit(p->var + i, name, sas->var_range[i]);
    }

    for (i = 0; i < sas->fact_size; ++i){
        if (sas->fact[i].var == PLAN_VAR_ID_UNDEFINED)
            continue;

        var = p->var + sas->fact[i].var;
        formatFactName(sas, ground, pddl, i, name);
        planVarSetValName(var, sas->fact[i].val, name);
    }

    for (i = 0; i < p->var_size; ++i){
        var = p->var + i;
        if (var->val[var->range - 1].name == NULL){
            planVarSetValName(var, var->range - 1, "<none of those>");
        }
    }

    return 0;
}

static int createStatePool(plan_problem_t *p, const plan_pddl_sas_t *sas,
                           const plan_pddl_ground_t *ground)
{
    plan_state_t *state;
    int i, fact_id;

    p->state_pool = planStatePoolNew(p->var, p->var_size);

    state = planStateNew(p->var_size);
    for (i = 0; i < p->var_size; ++i)
        planStateSet(state, i, p->var[i].range - 1);

    for (i = 0; i < ground->init.size; ++i){
        fact_id = ground->init.fact[i];
        if (sas->fact[fact_id].var != PLAN_VAR_ID_UNDEFINED){
            planStateSet(state, sas->fact[fact_id].var,
                                sas->fact[fact_id].val);
        }
    }
    p->initial_state = planStatePoolInsert(p->state_pool, state);
    planStateDel(state);

    p->goal = planPartStateNew(p->var_size);
    for (i = 0; i < ground->goal.size; ++i){
        fact_id = ground->goal.fact[i];
        if (sas->fact[fact_id].var == PLAN_VAR_ID_UNDEFINED){
            fprintf(stderr, "Problem Error: Invalid goal!\n");
            return -1;
        }

        planPartStateSet(p->goal, sas->fact[fact_id].var,
                                  sas->fact[fact_id].val);
    }

    return 0;
}

static int opCmp(const void *a, const void *b)
{
    const plan_op_t *o1 = a;
    const plan_op_t *o2 = b;
    return strcmp(o1->name, o2->name);
}

static void opSetPre(plan_op_t *op, const plan_pddl_sas_t *sas,
                     const plan_pddl_ground_action_t *action)
{
    int i, fact_id;
    plan_var_id_t var;
    plan_val_t val;

    for (i = 0; i < action->pre.size; ++i){
        fact_id = action->pre.fact[i];
        var = sas->fact[fact_id].var;
        val = sas->fact[fact_id].val;
        if (var == PLAN_VAR_ID_UNDEFINED)
            continue;

        planOpSetPre(op, var, val);
    }
}

static void opSetEff(plan_op_t *op, const plan_problem_t *p,
                     const plan_pddl_sas_t *sas,
                     const plan_pddl_ground_action_t *action)
{
    int set_var[p->var_size];
    int i, fact_id;
    plan_var_id_t var;
    plan_val_t val;

    bzero(set_var, sizeof(int) * p->var_size);

    for (i = 0; i < action->eff_add.size; ++i){
        fact_id = action->eff_add.fact[i];
        var = sas->fact[fact_id].var;
        val = sas->fact[fact_id].val;
        if (var == PLAN_VAR_ID_UNDEFINED)
            continue;
        if (set_var[var]){
            fprintf(stderr, "Problem Error: Conflicting variables in action"
                            " effect!!!\n");
        }

        planOpSetEff(op, var, val);
        set_var[var] = 1;
    }

    for (i = 0; i < action->eff_del.size; ++i){
        fact_id = action->eff_del.fact[i];
        var = sas->fact[fact_id].var;
        val = sas->fact[fact_id].val;
        if (var == PLAN_VAR_ID_UNDEFINED || set_var[var])
            continue;
        planOpSetEff(op, var, p->var[var].range - 1);
    }
}

static void opCondEffSetPre(plan_op_t *op, int ce_id,
                            const plan_pddl_sas_t *sas,
                            const plan_pddl_ground_cond_eff_t *ce)
{
    int i, fact_id;
    plan_var_id_t var;
    plan_val_t val;

    for (i = 0; i < ce->pre.size; ++i){
        fact_id = ce->pre.fact[i];
        var = sas->fact[fact_id].var;
        val = sas->fact[fact_id].val;
        if (var == PLAN_VAR_ID_UNDEFINED)
            continue;

        planOpCondEffSetPre(op, ce_id, var, val);
    }
}

static void opCondEffSetEff(plan_op_t *op, int ce_id,
                            const plan_problem_t *p,
                            const plan_pddl_sas_t *sas,
                            const plan_pddl_ground_cond_eff_t *ce)
{
    int set_var[p->var_size];
    int i, fact_id;
    plan_var_id_t var;
    plan_val_t val;

    bzero(set_var, sizeof(int) * p->var_size);

    for (i = 0; i < ce->eff_add.size; ++i){
        fact_id = ce->eff_add.fact[i];
        var = sas->fact[fact_id].var;
        val = sas->fact[fact_id].val;
        if (var == PLAN_VAR_ID_UNDEFINED)
            continue;
        if (set_var[var]){
            fprintf(stderr, "Problem Error: Conflicting variables in action"
                            " effect!!!\n");
        }

        planOpCondEffSetEff(op, ce_id, var, val);
        set_var[var] = 1;
    }

    for (i = 0; i < ce->eff_del.size; ++i){
        fact_id = ce->eff_del.fact[i];
        var = sas->fact[fact_id].var;
        val = sas->fact[fact_id].val;
        if (var == PLAN_VAR_ID_UNDEFINED || set_var[var])
            continue;
        planOpCondEffSetEff(op, ce_id, var, p->var[var].range - 1);
    }
}

static void opAddCondEff(plan_op_t *op, const plan_problem_t *p,
                         const plan_pddl_sas_t *sas,
                         const plan_pddl_ground_cond_eff_t *ce)
{
    int id;

    id = planOpAddCondEff(op);
    opCondEffSetPre(op, id, sas, ce);
    opCondEffSetEff(op, id, p, sas, ce);
}

static int setOp(plan_op_t *op, const plan_problem_t *p,
                 const plan_pddl_sas_t *sas,
                 const plan_pddl_ground_t *ground,
                 int metric,
                 const plan_pddl_ground_action_t *action)
{
    int i;

    planOpInit(op, p->var_size);
    planOpSetName(op, action->name);
    planOpSetCost(op, 1);
    if (metric)
        planOpSetCost(op, action->cost);

    opSetPre(op, sas, action);
    opSetEff(op, p, sas, action);
    if (op->eff->vals_size == 0){
        planOpFree(op);
        return -1;
    }

    // TODO: neg preconditions in conditional effects
    for (i = 0; i < action->cond_eff.size; ++i){
        if (action->cond_eff.cond_eff[i].pre_neg.size > 0){
            fprintf(stderr, "Problem Error: Negative preconditions in"
                            " conditional effects are not supported yet.\n");
            exit(-1);
        }
    }

    for (i = 0; i < action->cond_eff.size; ++i)
        opAddCondEff(op, p, sas, action->cond_eff.cond_eff + i);

    if (action->owner >= 0){
        planOpAddOwner(op, ground->obj_to_agent[action->owner]);
        op->owner = ground->obj_to_agent[action->owner];
    }

    return 0;
}

static void addOp(plan_problem_t *p, int *alloc_size,
                  const plan_pddl_sas_t *sas,
                  const plan_pddl_ground_t *ground,
                  int metric, const plan_pddl_ground_action_t *action)
{
    plan_op_t *op;

    if (p->op_size >= *alloc_size){
        *alloc_size *= 2;
        p->op = BOR_REALLOC_ARR(p->op, plan_op_t, *alloc_size);
    }

    op = p->op + p->op_size;
    if (setOp(op, p, sas, ground, metric, action) == 0)
        ++p->op_size;
}

/** Returns number of operators that will be created from this action that
 *  has negative precondition */
static int preNegNumOps(const plan_pddl_sas_t *sas,
                        const plan_pddl_ground_action_t *action)
{
    int i, fact_id, var, num = 1;

    for (i = 0; i < action->pre_neg.size; ++i){
        fact_id = action->pre_neg.fact[i];
        var = sas->fact[fact_id].var;
        if (var == PLAN_VAR_ID_UNDEFINED)
            continue;

        num *= sas->var_range[var] - 1;
    }

    return num;
}

static void addOpPreNegWrite(plan_problem_t *p, const plan_pddl_sas_t *sas,
                             const plan_pddl_ground_t *ground,
                             int metric, const plan_pddl_ground_action_t *action,
                             const plan_val_t *bound)
{
    plan_op_t *op;
    int i, fact_id, var, val;

    op = p->op + p->op_size;
    if (setOp(op, p, sas, ground, metric, action) != 0)
        return;

    ++p->op_size;
    for (i = 0; i < action->pre_neg.size; ++i){
        fact_id = action->pre_neg.fact[i];
        var = sas->fact[fact_id].var;
        val = bound[i];
        if (var != PLAN_VAR_ID_UNDEFINED)
            planOpSetPre(op, var, val);
    }
}

static void addOpPreNeg2(plan_problem_t *p, const plan_pddl_sas_t *sas,
                         const plan_pddl_ground_t *ground,
                         int metric, const plan_pddl_ground_action_t *action,
                         plan_val_t *bound, int pre_i)
{
    int i, fact_id;
    plan_var_id_t var;
    plan_val_t val;

    if (pre_i >= action->pre_neg.size){
        addOpPreNegWrite(p, sas, ground, metric, action, bound);
        return;
    }

    fact_id = action->pre_neg.fact[pre_i];
    var = sas->fact[fact_id].var;
    val = sas->fact[fact_id].val;
    if (var == PLAN_VAR_ID_UNDEFINED)
        addOpPreNeg2(p, sas, ground, metric, action, bound, pre_i + 1);

    for (i = 0; i < p->var[var].range; ++i){
        if (i == val)
            continue;
        bound[pre_i] = i;
        addOpPreNeg2(p, sas, ground, metric, action, bound, pre_i + 1);
    }
}

static void addOpPreNeg(plan_problem_t *p, int *alloc_size,
                        const plan_pddl_sas_t *sas,
                        const plan_pddl_ground_t *ground,
                        int metric, const plan_pddl_ground_action_t *action)
{
    plan_val_t bound[action->pre_neg.size];
    int num_ops;

    num_ops = preNegNumOps(sas, action);
    while (p->op_size + num_ops >= *alloc_size){
        *alloc_size *= 2;
        p->op = BOR_REALLOC_ARR(p->op, plan_op_t, *alloc_size);
    }

    addOpPreNeg2(p, sas, ground, metric, action, bound, 0);
}


static int cmpPartState(const plan_part_state_t *p1,
                        const plan_part_state_t *p2)
{
    int i;

    // First sort part-states with less values set
    if (p1->vals_size < p2->vals_size){
        return -1;
    }else if (p1->vals_size > p2->vals_size){
        return 1;
    }

    // We assume that in .vals_size are values sorted according to variable
    // ID
    for (i = 0; i < p1->vals_size; ++i){
        if (p1->vals[i].var != p2->vals[i].var){
            return p1->vals[i].var - p2->vals[i].var;
        }else if (p1->vals[i].val != p2->vals[i].val){
            return p1->vals[i].val - p2->vals[i].val;
        }
    }

    return 0;
}

static int cmpOp(const plan_op_t *op1, const plan_op_t *op2)
{
    int i, cmp;
    plan_op_cond_eff_t *ce1, *ce2;

    if (op1->pre->vals_size != op2->pre->vals_size)
        return op1->pre->vals_size - op2->pre->vals_size;
    if (op1->eff->vals_size != op2->eff->vals_size)
        return op1->eff->vals_size - op2->eff->vals_size;
    if (op1->cond_eff_size != op2->cond_eff_size)
        return op1->cond_eff_size - op2->cond_eff_size;

    if ((cmp = cmpPartState(op1->pre, op2->pre)) != 0)
        return cmp;
    if ((cmp = cmpPartState(op1->eff, op2->eff)) != 0)
        return cmp;

    for (i = 0; i < op1->cond_eff_size; ++i){
        ce1 = op1->cond_eff + i;
        ce2 = op2->cond_eff + i;
        if ((cmp = cmpPartState(ce1->pre, ce2->pre)) != 0)
            return cmp;
        if ((cmp = cmpPartState(ce1->eff, ce2->eff)) != 0)
            return cmp;
    }

    return 0;
}

static int duplicateOpsCmp(const void *a, const void *b)
{
    const plan_op_t *op1 = *(const plan_op_t **)a;
    const plan_op_t *op2 = *(const plan_op_t **)b;
    return cmpOp(op1, op2);
}

static void pruneDuplicateOps(plan_problem_t *prob)
{
    plan_op_t **sorted_ops;
    int i, ins;

    // Sort operators so that duplicates are one after other
    sorted_ops = BOR_ALLOC_ARR(plan_op_t *, prob->op_size);
    for (i = 0; i < prob->op_size; ++i)
        sorted_ops[i] = prob->op + i;
    qsort(sorted_ops, prob->op_size, sizeof(plan_op_t *), duplicateOpsCmp);

    // Free duplicate operators and mark their position with global_id set
    // to -1
    for (i = 1; i < prob->op_size; ++i){
        if (cmpOp(sorted_ops[i - 1], sorted_ops[i]) == 0){
            planOpFree(sorted_ops[i - 1]);
            sorted_ops[i - 1]->global_id = -1;
        }
    }

    // Squash operators to a continuous array
    for (i = 0, ins = 0; i < prob->op_size; ++i, ++ins){
        if (prob->op[i].global_id == -1){
            --ins;
        }else if (ins != i){
            prob->op[ins] = prob->op[i];
            prob->op[ins].global_id = ins;
        }
    }
    prob->duplicate_ops_removed = prob->op_size - ins;
    prob->op_size = ins;

    BOR_FREE(sorted_ops);
}

static void setVarValUsedByPartState(plan_problem_t *p,
                                     const plan_part_state_t *ps,
                                     int used_by)
{
    plan_var_id_t var;
    plan_val_t val;
    int i;

    PLAN_PART_STATE_FOR_EACH(ps, i, var, val){
        planVarSetValUsedBy(p->var + var, val, used_by);
    }
}

static void setVarValUsedByOp(plan_problem_t *p, const plan_op_t *op)
{
    int i;

    if (op->owner < 0)
        return;

    setVarValUsedByPartState(p, op->pre, op->owner);
    setVarValUsedByPartState(p, op->eff, op->owner);
    for (i = 0; i < op->cond_eff_size; ++i){
        setVarValUsedByPartState(p, op->cond_eff[i].pre, op->owner);
        setVarValUsedByPartState(p, op->cond_eff[i].eff, op->owner);
    }
}

static void setVarValUsedBy(plan_problem_t *p)
{
    int i;

    for (i = 0; i < p->op_size; ++i){
        if (p->op[i].owner >= 0)
            setVarValUsedByOp(p, p->op + i);
    }
}

static int isPartStatePrivate(const plan_part_state_t *ps,
                              const plan_var_t *pvar)
{
    plan_var_id_t var;
    plan_val_t val;
    int i;

    PLAN_PART_STATE_FOR_EACH(ps, i, var, val){
        if (!pvar[var].val[val].is_private)
            return 0;
    }

    return 1;
}

static int isOpPrivate(const plan_op_t *op, const plan_var_t *var)
{
    int i;

    if (!isPartStatePrivate(op->pre, var)
            || !isPartStatePrivate(op->eff, var))
        return 0;

    for (i = 0; i < op->cond_eff_size; ++i){
        if (!isPartStatePrivate(op->cond_eff[i].pre, var)
                || !isPartStatePrivate(op->cond_eff[i].eff, var))
            return 0;
    }

    return 1;
}

static void setOpPrivate(plan_op_t *op, const plan_var_t *var)
{
    if (isOpPrivate(op, var))
        op->is_private = 1;
}

static void setPrivateVarAndOp(plan_problem_t *p)
{
    plan_var_id_t var;
    plan_val_t val;
    int i;

    // Determine which agents use which values of variables
    setVarValUsedBy(p);

    // Set variables and their values as private according to the .used_by
    // bitarrays
    for (i = 0; i < p->var_size; ++i)
        planVarSetPrivateFromUsedBy(p->var + i);

    // Set goals as public
    PLAN_PART_STATE_FOR_EACH(p->goal, i, var, val){
        p->var[var].val[val].is_private = 0;
    }

    for (i = 0; i < p->op_size; ++i){
        if (p->op[i].owner >= 0)
            setOpPrivate(p->op + i, p->var);
    }
}

static int createOps(plan_problem_t *p, const plan_pddl_sas_t *sas,
                     const plan_pddl_ground_t *ground,
                     const plan_pddl_t *pddl, unsigned flags)
{
    const plan_pddl_ground_action_t *action;
    int i, metric, alloc_size;

    metric = pddl->metric;

    alloc_size = ground->action_pool.size;
    p->op = BOR_ALLOC_ARR(plan_op_t, alloc_size);
    p->op_size = 0;
    for (i = 0; i < ground->action_pool.size; ++i){
        action = planPDDLGroundActionPoolGet(&ground->action_pool, i);
        if (action->pre_neg.size > 0){
            addOpPreNeg(p, &alloc_size, sas, ground, metric, action);
        }else{
            addOp(p, &alloc_size, sas, ground, metric, action);
        }
    }

    // Remove duplicates
    if (flags & PLAN_PROBLEM_PRUNE_DUPLICATES)
        pruneDuplicateOps(p);

    // Give back unneeded memory and sort operators
    p->op = BOR_REALLOC_ARR(p->op, plan_op_t, p->op_size);
    qsort(p->op, p->op_size, sizeof(plan_op_t), opCmp);

    // Set global ID of all operators
    for (i = 0; i < p->op_size; ++i)
        p->op[i].global_id = i;

    // Set private variables and operators
    setPrivateVarAndOp(p);

    return 0;
}

static int createSuccGen(plan_problem_t *p)
{
    p->succ_gen = planSuccGenNew(p->op, p->op_size, NULL);
    return 0;
}

static int loadFromPDDL(plan_problem_t *p, const plan_pddl_sas_t *sas,
                        const plan_pddl_ground_t *ground,
                        const plan_pddl_t *pddl, unsigned flags)
{
    planProblemInit(p);
    p->ma_privacy_var = -1;

    if (createVar(p, sas, ground, pddl)
            || createStatePool(p, sas, ground) != 0
            || createOps(p, sas, ground, pddl, flags) != 0
            || createSuccGen(p) != 0){
        return -1;
    }

    return 0;
}

plan_problem_t *planProblemFromPDDL(const char *domain_pddl,
                                    const char *problem_pddl,
                                    unsigned flags)
{
    plan_problem_t *p;
    plan_pddl_t *pddl;
    plan_pddl_ground_t ground;
    plan_pddl_sas_t sas;
    unsigned sas_flags = 0;

    if (flags & PLAN_PROBLEM_USE_CG)
        sas_flags |= PLAN_PDDL_SAS_USE_CG;

    pddl = planPDDLNew(domain_pddl, problem_pddl);
    if (pddl == NULL)
        return NULL;

    planPDDLGround(&ground, pddl);
    planPDDLSas(&sas, &ground, sas_flags);

    p = BOR_ALLOC(plan_problem_t);
    if (loadFromPDDL(p, &sas, &ground, pddl, flags) != 0){
        planProblemDel(p);
        return NULL;
    }

    planPDDLSasFree(&sas);
    planPDDLGroundFree(&ground);
    planPDDLDel(pddl);

    planProblemPack(p);
    return p;
}


static void createAgentOps(plan_problem_t *dst, const plan_problem_t *src)
{
    int i;

    dst->op = BOR_CALLOC_ARR(plan_op_t, src->op_size);
    dst->op_size = 0;
    for (i = 0; i < src->op_size; ++i){
        if (src->op[i].owner == dst->agent_id){
            planOpInit(dst->op + dst->op_size, dst->var_size);
            planOpCopy(dst->op + dst->op_size, src->op + i);
            ++dst->op_size;
        }
    }

    if (dst->op_size != src->op_size)
        dst->op = BOR_REALLOC_ARR(dst->op, plan_op_t, dst->op_size);
}

static void projectPartState(const plan_problem_t *p, plan_part_state_t *ps)
{
    plan_var_id_t var;
    plan_val_t val;
    plan_var_id_t unset[ps->vals_size];
    int i, unset_size;

    bzero(unset, sizeof(plan_var_id_t) * ps->vals_size);
    unset_size = 0;
    PLAN_PART_STATE_FOR_EACH(ps, i, var, val){
        if (p->var[var].val[val].is_private
                && !planVarValIsUsedBy(p->var + var, val, p->agent_id))
            unset[unset_size++] = var;
    }

    for (i = 0; i < unset_size; ++i)
        planPartStateUnset(ps, unset[i]);
}

static int projectOp(const plan_problem_t *p, plan_op_t *op)
{
    int i;

    projectPartState(p, op->pre);
    projectPartState(p, op->eff);

    for (i = 0; i < op->cond_eff_size; ++i){
        // TODO: Probably we should delete cond-eff without effect and do
        //       something else with cond-eff without precondition.
        projectPartState(p, op->cond_eff[i].pre);
        projectPartState(p, op->cond_eff[i].eff);
    }

    if (op->eff->vals_size > 0 || op->pre->vals_size > 0
            || op->cond_eff_size > 0)
        return 1;
    return 0;
}

static void createAgentProjOps(plan_problem_t *dst, const plan_problem_t *src)
{
    int i, ok;

    dst->proj_op = BOR_CALLOC_ARR(plan_op_t, src->op_size);
    dst->proj_op_size = 0;
    for (i = 0; i < src->op_size; ++i){
        // Skip private operators of other agents
        if (src->op[i].is_private && src->op[i].owner != dst->agent_id)
            continue;

        // Copy full operator
        planOpInit(dst->proj_op + dst->proj_op_size, dst->var_size);
        planOpCopy(dst->proj_op + dst->proj_op_size, src->op + i);

        // Project it if it is other agent's operator
        ok = 1;
        if (src->op[i].owner != dst->agent_id)
            ok = projectOp(dst, dst->proj_op + dst->proj_op_size);

        if (ok){
            ++dst->proj_op_size;
        }else{
            planOpFree(dst->proj_op + dst->proj_op_size);
        }
    }

    if (dst->proj_op_size != src->op_size){
        dst->proj_op = BOR_REALLOC_ARR(dst->proj_op, plan_op_t,
                                       dst->proj_op_size);
    }
}

static void initAgentProblem(plan_problem_t *p, int id,
                             const plan_pddl_ground_t *ground,
                             const plan_pddl_t *pddl)
{
    const char *name;

    name = pddl->obj.obj[ground->agent_to_obj[id]].name;

    bzero(p, sizeof(*p));
    planProblemInit(p);
    p->agent_id = id;
    p->num_agents = ground->agent_size;
    p->agent_name = BOR_STRDUP(name);
}

static void createAgentProblem(plan_problem_t *dst,
                               const plan_problem_t *src,
                               unsigned flags)
{
    plan_state_t *state;
    int i;

    dst->var_size = src->var_size;
    dst->var = BOR_ALLOC_ARR(plan_var_t, dst->var_size);
    for (i = 0; i < dst->var_size; ++i)
        planVarCopy(dst->var + i, src->var + i);
    dst->ma_privacy_var = src->ma_privacy_var;

    dst->state_pool = planStatePoolNew(dst->var, dst->var_size);
    state = planStateNew(src->state_pool->num_vars);
    planStatePoolGetState(src->state_pool, src->initial_state, state);
    dst->initial_state = planStatePoolInsert(dst->state_pool, state);
    planStateDel(state);

    dst->goal = planPartStateNew(dst->state_pool->num_vars);
    planPartStateCopy(dst->goal, src->goal);

    createAgentOps(dst, src);
    createAgentProjOps(dst, src);

    dst->succ_gen = planSuccGenNew(dst->op, dst->op_size, NULL);
}

plan_problem_agents_t *planProblemUnfactorFromPDDL(const char *domain_pddl,
                                                   const char *problem_pddl,
                                                   unsigned flags)
{
    plan_problem_agents_t *p;
    plan_pddl_t *pddl;
    plan_pddl_ground_t ground;
    plan_pddl_sas_t sas;
    unsigned sas_flags = 0;
    int i;

    if (flags & PLAN_PROBLEM_USE_CG)
        sas_flags |= PLAN_PDDL_SAS_USE_CG;

    pddl = planPDDLNew(domain_pddl, problem_pddl);
    if (pddl == NULL)
        return NULL;

    planPDDLGround(&ground, pddl);
    planPDDLSas(&sas, &ground, sas_flags);

    p = BOR_ALLOC(plan_problem_agents_t);
    bzero(p, sizeof(*p));

    if (loadFromPDDL(&p->glob, &sas, &ground, pddl, flags) != 0){
        planProblemAgentsDel(p);
        return NULL;
    }

    p->agent_size = ground.agent_size;
    p->agent = BOR_CALLOC_ARR(plan_problem_t, p->agent_size);
    for (i = 0; i < p->agent_size; ++i){
        initAgentProblem(p->agent + i, i, &ground, pddl);
        createAgentProblem(p->agent + i, &p->glob, flags);
    }

    planPDDLSasFree(&sas);
    planPDDLGroundFree(&ground);
    planPDDLDel(pddl);

    planProblemAgentsPack(p);
    return p;
}
