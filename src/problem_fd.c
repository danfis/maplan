#include <boruvka/alloc.h>
#include "plan/problem.h"

static int loadFD(plan_problem_t *plan, const char *filename);

plan_problem_t *planProblemFromFD(const char *fn)
{
    plan_problem_t *p;

    p = BOR_ALLOC(plan_problem_t);
    planProblemInit(p);

    if (loadFD(p, fn) != 0){
        planProblemFree(p);
        BOR_FREE(p);
        return NULL;
    }

    return p;
}


static int fdAssert(FILE *fin, const char *str)
{
    char read_s[1024];
    if (fscanf(fin, "%s", read_s) != 1)
        return -1;
    if (strcmp(read_s, str) != 0)
        return -1;
    return 0;
}

static int fdVersion(plan_problem_t *plan, FILE *fin)
{
    int version;

    if (fdAssert(fin, "begin_version") != 0)
        return -1;

    if (fscanf(fin, "%d", &version) != 1)
        return -1;

    if (version != 3){
        fprintf(stderr, "Error: Unknown version.\n");
        return -1;
    }

    if (fdAssert(fin, "end_version") != 0)
        return -1;

    return 0;
}

static int fdMetric(FILE *fin)
{
    int val;

    if (fdAssert(fin, "begin_metric") != 0)
        return -1;
    if (fscanf(fin, "%d", &val) != 1)
        return -1;
    if (fdAssert(fin, "end_metric") != 0)
        return -1;
    return val;
}

static int fdVar1(plan_var_t *var, FILE *fin)
{
    plan_val_t i;
    char sval[1024], *fact_name;
    size_t size;
    int layer, range;

    if (fdAssert(fin, "begin_variable") != 0)
        return -1;
    if (fscanf(fin, "%s %d %d", sval, &layer, &range) != 3)
        return -1;

    planVarInit(var, sval, range);
    if ((int)var->range != range){
        fprintf(stderr, "Error: Could not load variable %s, because the\n"
                        "range can't be stored in %d bytes long variable.\n"
                        "Change definition of plan_val_t and recompile.\n",
                        var->name, (int)sizeof(var->range));
        return -1;
    }

    fact_name = NULL;
    size = 0;
    getline(&fact_name, &size, fin);
    for (i = 0; i < var->range; ++i){
        getline(&fact_name, &size, fin);
    }

    if (fact_name)
        free(fact_name);

    if (fdAssert(fin, "end_variable") != 0)
        return -1;

    return 0;
}

static int fdVars(plan_problem_t *plan, FILE *fin)
{
    int i, num_vars;

    if (fscanf(fin, "%d", &num_vars) != 1)
        return -1;

    plan->var_size = num_vars;
    plan->var = BOR_CALLOC_ARR(plan_var_t, plan->var_size);
    for (i = 0; i < plan->var_size; ++i){
        fdVar1(plan->var + i, fin);
    }

    // create state pool
    plan->state_pool = planStatePoolNew(plan->var, plan->var_size);

    return 0;
}

static int fdMutexes(plan_problem_t *plan, FILE *fin)
{
    int i, j, len;
    int num_facts, var, val;

    if (fscanf(fin, "%d", &len) != 1)
        return -1;

    for (i = 0; i < len; ++i){
        if (fdAssert(fin, "begin_mutex_group") != 0)
            return -1;
        if (fscanf(fin, "%d", &num_facts) != 1)
            return -1;

        for (j = 0; j < num_facts; ++j){
            if (fscanf(fin, "%d %d", &var, &val) != 2)
                return -1;
        }

        if (fdAssert(fin, "end_mutex_group") != 0)
            return -1;
    }

    return 0;
}

static int fdInitState(plan_problem_t *plan, FILE *fin)
{
    plan_state_t *state;
    int var, val;

    if (fdAssert(fin, "begin_state") != 0)
        return -1;

    state = planStateNew(plan->state_pool->num_vars);
    for (var = 0; var < plan->var_size; ++var){
        if (fscanf(fin, "%d", &val) != 1)
            return -1;
        planStateSet(state, var, val);
    }
    plan->initial_state = planStatePoolInsert(plan->state_pool, state);

    planStateDel(state);

    if (fdAssert(fin, "end_state") != 0)
        return -1;

    return 0;
}

static int fdGoal(plan_problem_t *plan, FILE *fin)
{
    int i, len, var, val;

    if (fdAssert(fin, "begin_goal") != 0)
        return -1;

    if (fscanf(fin, "%d", &len) != 1)
        return -1;

    plan->goal = planPartStateNew(plan->state_pool->num_vars);
    for (i = 0; i < len; ++i){
        if (fscanf(fin, "%d %d", &var, &val) != 2)
            return -1;
        planPartStateSet(plan->goal, var, val);
    }

    if (fdAssert(fin, "end_goal") != 0)
        return -1;

    return 0;
}

static int fdOperator(plan_op_t *op, FILE *fin, int use_metric)
{
    char *name;
    size_t name_size;
    ssize_t name_len;
    int i, len, var, cond, ci, pre, post, cost;
    int cond_eff_id = 0;

    if (fdAssert(fin, "begin_operator") != 0)
        return -1;

    name = NULL;
    name_size = 0;
    getline(&name, &name_size, fin);
    name_len = getline(&name, &name_size, fin);
    if (name_len > 0)
        name[name_len - 1] = 0x0;
    op->name = strdup(name);

    if (name)
        free(name);

    // prevail
    if (fscanf(fin, "%d", &len) != 1)
        return -1;
    for (i = 0; i < len; ++i){
        if (fscanf(fin, "%d %d", &var, &pre) != 2)
            return -1;
        planOpSetPre(op, var, pre);
    }

    // pre-post
    if (fscanf(fin, "%d", &len) != 1)
        return -1;
    for (i = 0; i < len; ++i){
        if (fscanf(fin, "%d", &cond) != 1)
            return -1;
        if (cond > 0){
            cond_eff_id = planOpAddCondEff(op);
            for (ci = 0; ci < cond; ++ci){
                if (fscanf(fin, "%d %d", &var, &pre) != 2)
                    return -1;
                planOpCondEffSetPre(op, cond_eff_id, var, pre);
            }
        }

        if (fscanf(fin, "%d %d %d", &var, &pre, &post) != 3)
            return -1;

        if (pre != -1)
            planOpSetPre(op, var, pre);

        if (cond == 0){
            planOpSetEff(op, var, post);
        }else{
            planOpCondEffSetEff(op, cond_eff_id, var, post);
        }
    }

    planOpCondEffSimplify(op);

    if (fscanf(fin, "%d", &cost) != 1)
        return -1;
    op->cost = (use_metric ? cost : 1);

    if (fdAssert(fin, "end_operator") != 0)
        return -1;

    return 0;
}

static int fdOperators(plan_problem_t *plan, FILE *fin, int use_metric)
{
    int i, num_ops;

    if (fscanf(fin, "%d", &num_ops) != 1)
        return -1;

    plan->op_size = num_ops;
    plan->op = BOR_ALLOC_ARR(plan_op_t, plan->op_size);
    for (i = 0; i < plan->op_size; ++i){
        planOpInit(plan->op + i, plan->state_pool->num_vars);
    }

    for (i = 0; i < num_ops; ++i){
        if (fdOperator(plan->op + i, fin, use_metric) != 0)
            return -1;
    }

    return 0;
}

static int fdAxioms(plan_problem_t *plan, FILE *fin)
{
    int i, len;
    char *line;
    size_t line_size;
    ssize_t line_len;

    if (fscanf(fin, "%d", &len) != 1)
        return -1;

    if (len > 0){
        fprintf(stderr, "Error: Axioms are not supported!\n");
        return -1;
    }

    line = NULL;
    line_size = 0;
    for (i = 0; i < len; ++i){
        while (1){
            line_len = getline(&line, &line_size, fin);
            if (line_len > 1){
                line[line_len - 1] = 0;
                if (strcmp(line, "end_operator") == 0)
                    break;
            }
        }
    }

    return 0;
}

static int loadFDBase(plan_problem_t *plan, FILE *fin,
                      int *use_metric_out)
{
    int use_metric;

    if (fdVersion(plan, fin) != 0)
        return -1;
    if ((use_metric = fdMetric(fin)) < 0)
        return -1;
    if (fdVars(plan, fin) != 0)
        return -1;
    if (fdMutexes(plan, fin) != 0)
        return -1;
    if (fdInitState(plan, fin) != 0)
        return -1;
    if (fdGoal(plan, fin) != 0)
        return -1;
    if (fdOperators(plan, fin, use_metric) != 0)
        return -1;
    if (fdAxioms(plan, fin) != 0)
        return -1;

    if (fdAssert(fin, "begin_SG") == 0){
        plan->succ_gen = planSuccGenFromFD(fin, plan->var, plan->op);
        if (fdAssert(fin, "end_SG") != 0)
            return -1;
    }else{
        plan->succ_gen = planSuccGenNew(plan->op, plan->op_size, NULL);
    }

    if (use_metric_out)
        *use_metric_out = use_metric;

    return 0;
}

static int loadFD(plan_problem_t *plan, const char *filename)
{
    FILE *fin;

    fin = fopen(filename, "r");
    if (fin == NULL){
        fprintf(stderr, "Error: Could not read `%s'.\n", filename);
        return -1;
    }

    if (loadFDBase(plan, fin, NULL) != 0)
        return -1;

    fclose(fin);

    return 0;
}
