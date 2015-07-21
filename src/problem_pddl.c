#include <boruvka/alloc.h>
#include <plan/problem.h>
#include <plan/pddl.h>
#include <plan/pddl_ground.h>
#include <plan/pddl_sas.h>

static void formatFactName(const plan_pddl_sas_t *sas, int fact_id, char *dst)
{
    const plan_pddl_fact_t *fact;
    const plan_pddl_predicate_t *pred;
    plan_pddl_fact_pool_t *fact_pool;
    int i, len;

    fact_pool = (plan_pddl_fact_pool_t *)&sas->ground->fact_pool;
    fact = planPDDLFactPoolGet(fact_pool, fact_id);
    pred = sas->ground->pddl->predicate.pred + fact->pred;

    len = sprintf(dst, "%s(", pred->name);
    dst += len;

    for (i = 0; i < fact->arg_size; ++i){
        if (i > 0){
            len = sprintf(dst, ", ");
            dst += len;
        }

        len = sprintf(dst, "%s", sas->ground->pddl->obj.obj[fact->arg[i]].name);
        dst += len;
    }

    sprintf(dst, ")");
}

static void createVar(plan_problem_t *p, const plan_pddl_sas_t *sas)
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
        formatFactName(sas, i, name);
        planVarSetValName(var, sas->fact[i].val, name);
    }

    for (i = 0; i < p->var_size; ++i){
        var = p->var + i;
        if (var->val_name[var->range - 1] == NULL){
            planVarSetValName(var, var->range - 1, "<none of those>");
        }
    }
}

static int createStatePool(plan_problem_t *p, const plan_pddl_sas_t *sas)
{
    plan_state_t *state;
    int i, fact_id;

    p->state_pool = planStatePoolNew(p->var, p->var_size);

    state = planStateNew(p->var_size);
    for (i = 0; i < p->var_size; ++i)
        planStateSet(state, i, p->var[i].range - 1);

    for (i = 0; i < sas->init.size; ++i){
        fact_id = sas->init.fact[i];
        if (sas->fact[fact_id].var != PLAN_VAR_ID_UNDEFINED){
            planStateSet(state, sas->fact[fact_id].var,
                                sas->fact[fact_id].val);
        }
    }
    p->initial_state = planStatePoolInsert(p->state_pool, state);
    planStateDel(state);

    p->goal = planPartStateNew(p->var_size);
    for (i = 0; i < sas->goal.size; ++i){
        fact_id = sas->goal.fact[i];
        if (sas->fact[fact_id].var == PLAN_VAR_ID_UNDEFINED){
            fprintf(stderr, "Problem Error: Invalid goal!\n");
            return -1;
        }

        planPartStateSet(p->goal, sas->fact[fact_id].var,
                                  sas->fact[fact_id].val);
    }

    return 0;
}

plan_problem_t *planProblemFromPDDL(const char *domain_pddl,
                                    const char *problem_pddl,
                                    unsigned flags)
{
    plan_pddl_t *pddl;
    plan_pddl_ground_t ground;
    plan_pddl_sas_t sas;
    plan_problem_t *p;
    unsigned sas_flags = 0;

    if (flags & PLAN_PROBLEM_USE_CG)
        sas_flags |= PLAN_PDDL_SAS_USE_CG;

    pddl = planPDDLNew(domain_pddl, problem_pddl);
    if (pddl == NULL)
        return NULL;

    planPDDLGroundInit(&ground, pddl);
    planPDDLGround(&ground);
    planPDDLSasInit(&sas, &ground);
    planPDDLSas(&sas, sas_flags);

    p = BOR_CALLOC_ARR(plan_problem_t, 1);
    createVar(p, &sas);
    if (createStatePool(p, &sas) != 0){
        planProblemDel(p);
        return NULL;
    }

    planPDDLSasFree(&sas);
    planPDDLGroundFree(&ground);
    planPDDLDel(pddl);
    return p;
}
