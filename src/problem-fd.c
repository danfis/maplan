#include <boruvka/alloc.h>
#include "plan/problem.h"

static int loadFD(plan_problem_t *plan, const char *filename);
static int loadAgentFD(plan_problem_agents_t *agents, const char *fn);

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

plan_problem_agents_t *planProblemAgentsFromFD(const char *fn)
{
    plan_problem_agents_t *agents;

    agents = BOR_ALLOC(plan_problem_agents_t);
    agents->agent = NULL;
    agents->agent_size = 0;

    if (loadAgentFD(agents, fn) != 0){
        BOR_FREE(agents);
        return NULL;
    }

    return agents;
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

    var->name = strdup(sval);
    var->range = range;

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
    plan->var = BOR_ALLOC_ARR(plan_var_t, plan->var_size);
    for (i = 0; i < plan->var_size; ++i){
        planVarInit(plan->var + i);
    }

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

    state = planStateNew(plan->state_pool);
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

    plan->goal = planPartStateNew(plan->state_pool);
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
        planOpInit(plan->op + i, plan->state_pool);
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

static int fdDomainTransitionGraph(plan_problem_t *plan, FILE *fin)
{
    char *line = NULL;
    size_t size = 0;
    int i;

    // Ignore domain transition graph -- just skip it
    for (i = 0; i < plan->var_size; ++i){
        if (fdAssert(fin, "begin_DTG") != 0)
            return -1;

        while (getline(&line, &size, fin) > 0
                && strcmp(line, "end_DTG\n") != 0);
    }

    if (line)
        free(line);

    return 0;
}

static int fdCausalGraph(plan_problem_t *plan, FILE *fin)
{
    char *line = NULL;
    size_t size = 0;

    // Skip causal graph for now.
    if (fdAssert(fin, "begin_CG") != 0)
        return -1;
    while (getline(&line, &size, fin) > 0
            && strcmp(line, "end_CG\n") != 0);
    if (line)
        free(line);

    return 0;
}

static void agentInitProblem(plan_problem_t *dst, const plan_problem_t *src)
{
    int i;
    plan_state_t *state;
    plan_var_id_t var;
    plan_val_t val;

    planProblemInit(dst);

    dst->var_size = src->var_size;
    dst->var = BOR_ALLOC_ARR(plan_var_t, src->var_size);
    for (i = 0; i < src->var_size; ++i){
        planVarCopy(dst->var + i, src->var + i);
    }

    if (!src->state_pool)
        return;

    dst->state_pool = planStatePoolNew(dst->var, dst->var_size);

    state = planStateNew(src->state_pool);
    planStatePoolGetState(src->state_pool, src->initial_state, state);
    dst->initial_state = planStatePoolInsert(dst->state_pool, state);
    planStateDel(state);

    dst->goal = planPartStateNew(dst->state_pool);
    PLAN_PART_STATE_FOR_EACH(src->goal, i, var, val){
        planPartStateSet(dst->goal, var, val);
    }

    dst->op_size = 0;
    dst->op = NULL;
    dst->succ_gen = NULL;
}

static int agentLoadPrivateOperators(plan_problem_t *prob,
                                     const plan_op_t *src_op,
                                     FILE *fin)
{
    int i, op_size, op_id, ins_from;

    if (fdAssert(fin, "begin_agent_private_operators") != 0)
        return -1;

    ins_from = prob->op_size;

    if (fscanf(fin, "%d", &op_size) != 1)
        return -1;
    prob->op_size += op_size;
    prob->op = BOR_REALLOC_ARR(prob->op, plan_op_t, prob->op_size);

    for (i = 0; i < op_size; ++i){
        if (fscanf(fin, "%d", &op_id) != 1)
            return -1;

        planOpInit(prob->op + ins_from, prob->state_pool);
        planOpCopy(prob->op + ins_from, src_op + op_id);
        planOpExtraMAOpSetPrivate(prob->op + ins_from);
        ++ins_from;
    }

    if (fdAssert(fin, "end_agent_private_operators") != 0)
        return -1;

    return 0;
}

static int agentLoadPublicOperators(int agent_id,
                                    plan_problem_t *prob,
                                    const plan_op_t *src_op,
                                    FILE *fin)
{
    int i, op_size, op_id, ins_from;
    int p, peer_size, peer;

    if (fdAssert(fin, "begin_agent_public_operators") != 0)
        return -1;

    ins_from = prob->op_size;

    if (fscanf(fin, "%d", &op_size) != 1)
        return -1;
    prob->op_size += op_size;
    prob->op = BOR_REALLOC_ARR(prob->op, plan_op_t, prob->op_size);

    for (i = 0; i < op_size; ++i){
        if (fscanf(fin, "%d", &op_id) != 1)
            return -1;

        planOpInit(prob->op + ins_from, prob->state_pool);
        planOpCopy(prob->op + ins_from, src_op + op_id);

        if (fscanf(fin, "%d", &peer_size) != 1)
            return -1;
        for (p = 0; p < peer_size; ++p){
            if (fscanf(fin, "%d", &peer) != 1)
                return -1;
            if (peer != agent_id)
                planOpExtraMAOpAddRecvAgent(prob->op + ins_from, peer);
        }

        ++ins_from;
    }

    if (fdAssert(fin, "end_agent_public_operators") != 0)
        return -1;

    return 0;
}

static int fdAgents(const plan_problem_t *prob, FILE *fin,
                    plan_problem_t **agents_out, int *num_agents_out,
                    int use_metric)
{
    plan_problem_t *agents;
    plan_problem_t *agent;
    int i, opi, num_agents, owner, global_id, agent_id;
    char name[1024];

    if (fscanf(fin, "%d", &num_agents) != 1)
        return 0;

    agents = BOR_ALLOC_ARR(plan_problem_t, num_agents);

    for (agent_id = 0; agent_id < num_agents; ++agent_id){
        if (fdAssert(fin, "begin_agent") != 0)
            return -1;


        agent = agents + agent_id;
        agentInitProblem(agent, prob);

        if (fscanf(fin, "%s %d", name, &i) != 2 || i != agent_id)
            return -1;
        agent->agent_name = strdup(name);

        if (agentLoadPrivateOperators(agent, prob->op, fin) != 0)
            return -1;
        if (agentLoadPublicOperators(agent_id, agent, prob->op, fin) != 0)
            return -1;

        agent->succ_gen = planSuccGenNew(agent->op, agent->op_size, NULL);


        if (fdAssert(fin, "begin_agent_projected_operators") != 0)
            return -1;

        if (fscanf(fin, "%d", &agent->proj_op_size) != 1)
            return -1;
        agent->proj_op = BOR_ALLOC_ARR(plan_op_t, agent->proj_op_size);

        for (opi = 0; opi < agent->proj_op_size; ++opi){
            planOpInit(agent->proj_op + opi, agent->state_pool);
            fdOperator(agent->proj_op + opi, fin, use_metric);

            if (fscanf(fin, "%d %d", &global_id, &owner) != 2)
                return -1;
            planOpExtraMAProjOpSetGlobalId(agent->proj_op + opi, global_id);
            planOpExtraMAProjOpSetOwner(agent->proj_op + opi, owner);
        }

        if (fdAssert(fin, "end_agent_projected_operators") != 0)
            return -1;

        if (fdAssert(fin, "end_agent") != 0)
            return -1;
    }

    *num_agents_out = num_agents;
    *agents_out = agents;
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

    if (fdAssert(fin, "begin_SG") != 0)
        return -1;
    plan->succ_gen = planSuccGenFromFD(fin, plan->var, plan->op);
    if (fdAssert(fin, "end_SG") != 0)
        return -1;

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

static int loadAgentFD(plan_problem_agents_t *aprob,
                       const char *filename)
{
    FILE *fin;
    int use_metric;

    fin = fopen(filename, "r");
    if (fin == NULL){
        fprintf(stderr, "Error: Could not read `%s'.\n", filename);
        return -1;
    }

    planProblemInit(&aprob->glob);

    if (loadFDBase(&aprob->glob, fin, &use_metric) != 0)
        return -1;

    if (fdDomainTransitionGraph(&aprob->glob, fin) != 0)
        return -1;
    if (fdCausalGraph(&aprob->glob, fin) != 0)
        return -1;
    if (fdAgents(&aprob->glob, fin, &aprob->agent, &aprob->agent_size,
                 use_metric) != 0)
        return -1;

    fclose(fin);

    if (aprob->agent_size == 0){
        planProblemFree(&aprob->glob);
        return -1;
    }

    return 0;
}
