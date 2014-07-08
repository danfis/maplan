#include <string.h>
#include <jansson.h>

#include <boruvka/alloc.h>

#include "plan/problem.h"

#define READ_BUFSIZE 1024


static void planProblemFree(plan_problem_t *plan)
{
    int i;

    if (plan->succ_gen)
        planSuccGenDel(plan->succ_gen);

    if (plan->goal){
        planPartStateDel(plan->state_pool, plan->goal);
        plan->goal = NULL;
    }

    if (plan->var != NULL){
        for (i = 0; i < plan->var_size; ++i){
            planVarFree(plan->var + i);
        }
        BOR_FREE(plan->var);
    }
    plan->var = NULL;

    if (plan->state_pool)
        planStatePoolDel(plan->state_pool);
    plan->state_pool = NULL;

    if (plan->op){
        for (i = 0; i < plan->op_size; ++i){
            planOperatorFree(plan->op + i);
        }
        BOR_FREE(plan->op);
    }
    plan->op = NULL;
    plan->op_size = 0;
}

static void planProblemInit(plan_problem_t *p)
{
    p->var = NULL;
    p->var_size = 0;
    p->state_pool = NULL;
    p->op = NULL;
    p->op_size = 0;
    p->goal = NULL;
    p->succ_gen = NULL;
}

static int loadJson(plan_problem_t *plan, const char *filename);
static int loadFD(plan_problem_t *plan, const char *filename);
static int loadAgentFD(const char *fn,
                       plan_problem_agent_t **agents,
                       int *num_agents);

plan_problem_t *planProblemFromJson(const char *fn)
{
    plan_problem_t *p;

    p = BOR_ALLOC(plan_problem_t);
    planProblemInit(p);

    if (loadJson(p, fn) != 0){
        planProblemFree(p);
        BOR_FREE(p);
        return NULL;
    }

    return p;
}

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

void planProblemDel(plan_problem_t *plan)
{
    planProblemFree(plan);
    BOR_FREE(plan);
}

plan_problem_agents_t *planProblemAgentsFromFD(const char *fn)
{
    plan_problem_agents_t *agents;

    agents = BOR_ALLOC(plan_problem_agents_t);
    agents->agent = NULL;
    agents->agent_size = 0;

    if (loadAgentFD(fn, &agents->agent, &agents->agent_size) != 0){
        BOR_FREE(agents);
        return NULL;
    }

    return agents;
}

static void agentFree(plan_problem_agent_t *ag)
{
    int i;

    planProblemFree(&ag->prob);
    if (ag->name)
        BOR_FREE(ag->name);

    if (ag->projected_op){
        for (i = 0; i < ag->projected_op_size; ++i){
            planOperatorFree(ag->projected_op + i);
        }
        BOR_FREE(ag->projected_op);
    }
}

void planProblemAgentsDel(plan_problem_agents_t *agents)
{
    int i;

    for (i = 0; i < agents->agent_size; ++i){
        agentFree(agents->agent + i);
    }
    if (agents->agent)
        BOR_FREE(agents->agent);
    BOR_FREE(agents);
}


static int loadJsonVersion(plan_problem_t *plan, json_t *json)
{
    int version = 0;
    version = json_integer_value(json);
    if (version != 3){
        fprintf(stderr, "Error: Unknown version.\n");
        return -1;
    }
    return 0;
}

static int loadJsonVariable1(plan_var_t *var, json_t *json)
{
    json_t *data, *jval;
    plan_val_t i;
    int range;

    data = json_object_get(json, "name");
    var->name = strdup(json_string_value(data));

    data = json_object_get(json, "range");
    range = json_integer_value(data);
    var->range = range;

    if ((int)var->range != range){
        fprintf(stderr, "Error: Could not load variable %s, because the\n"
                        "range can't be stored in %d bytes long variable.\n"
                        "Change definition of plan_val_t and recompile.\n",
                        var->name, (int)sizeof(var->range));
        return -1;
    }

    data = json_object_get(json, "layer");
    var->axiom_layer = json_integer_value(data);

    var->fact_name = BOR_ALLOC_ARR(char *, var->range);
    data = json_object_get(json, "fact_name");
    for (i = 0; i < var->range; i++){
        if (i < (int)json_array_size(data)){
            jval = json_array_get(data, i);
            var->fact_name[i] = strdup(json_string_value(jval));
        }else{
            var->fact_name[i] = strdup("");
        }
    }

    return 0;
}

static int loadJsonVariable(plan_problem_t *plan, json_t *json)
{
    int i;
    json_t *json_var;

    // allocate array for variables
    plan->var_size = json_array_size(json);
    plan->var = BOR_ALLOC_ARR(plan_var_t, plan->var_size);
    for (i = 0; i < plan->var_size; ++i){
        planVarInit(plan->var + i);
    }

    json_array_foreach(json, i, json_var){
        if (loadJsonVariable1(plan->var + i, json_var) != 0)
            return -1;
    }

    // create state pool
    plan->state_pool = planStatePoolNew(plan->var, plan->var_size);

    return 0;
}

static int loadJsonInitialState(plan_problem_t *plan, json_t *json)
{
    int i, len;
    json_t *json_val;
    plan_val_t val;
    plan_state_t *state;

    // check we have correct size of the state
    len = json_array_size(json);
    if (len != plan->var_size){
        fprintf(stderr, "Error: Invalid number of variables in "
                        "initial state.\n");
        return -1;
    }

    // create a new state structure
    state = planStateNew(plan->state_pool);

    // set up state variable values
    json_array_foreach(json, i, json_val){
        val = json_integer_value(json_val);
        planStateSet(state, i, val);
    }

    // create a new state and remember its ID
    plan->initial_state = planStatePoolInsert(plan->state_pool, state);

    // destroy the previously created state
    planStateDel(plan->state_pool, state);

    return 0;
}

static int loadJsonGoal(plan_problem_t *plan, json_t *json)
{
    const char *key;
    json_t *json_val;
    plan_var_id_t var;
    plan_val_t val;

    // allocate a new state
    plan->goal = planPartStateNew(plan->state_pool);

    json_object_foreach(json, key, json_val){
        var = atoi(key);
        val = json_integer_value(json_val);
        planPartStateSet(plan->state_pool, plan->goal, var, val);
    }

    return 0;
}

static int loadJsonOperator1(plan_operator_t *op, json_t *json)
{
    const char *key;
    json_t *data, *jprepost, *jpre, *jpost, *jprevail;
    plan_var_id_t var;
    int pre, post;

    data = json_object_get(json, "name");
    planOperatorSetName(op, json_string_value(data));

    data = json_object_get(json, "cost");
    planOperatorSetCost(op, json_integer_value(data));

    data = json_object_get(json, "pre_post");
    json_object_foreach(data, key, jprepost){
        var = atoi(key);

        jpre  = json_object_get(jprepost, "pre");
        jpost = json_object_get(jprepost, "post");

        pre  = json_integer_value(jpre);
        post = json_integer_value(jpost);

        if (pre != -1)
            planOperatorSetPrecondition(op, var, pre);
        planOperatorSetEffect(op, var, post);
    }

    data = json_object_get(json, "prevail");
    json_object_foreach(data, key, jprevail){
        var = atoi(key);
        pre = json_integer_value(jprevail);

        planOperatorSetPrecondition(op, var, pre);
    }

    return 0;
}

static int loadJsonOperator(plan_problem_t *plan, json_t *json)
{
    int i;
    json_t *json_op;

    // allocate array for operators
    plan->op_size = json_array_size(json);
    plan->op = BOR_ALLOC_ARR(plan_operator_t, plan->op_size);
    for (i = 0; i < plan->op_size; ++i){
        planOperatorInit(plan->op + i, plan->state_pool);
    }

    // set up all operators
    json_array_foreach(json, i, json_op){
        if (loadJsonOperator1(plan->op + i, json_op) != 0)
            return -1;
    }

    return 0;
}

typedef int (*load_json_data_fn)(plan_problem_t *plan, json_t *json);
static int loadJsonData(plan_problem_t *plan, json_t *root,
                         const char *keyname, load_json_data_fn fn)
{
    json_t *data;

    data = json_object_get(root, keyname);
    if (data == NULL){
        fprintf(stderr, "Error: No '%s' key in json definition.\n", keyname);
        return -1;
    }
    return fn(plan, data);
}

static int loadJson(plan_problem_t *plan, const char *filename)
{
    json_t *json, *jsuccgen;
    json_error_t json_err;

    // open JSON data from file
    json = json_load_file(filename, 0, &json_err);
    if (json == NULL){
        // TODO: Error logging
        fprintf(stderr, "Error: Could not read json data. Line %d: %s.\n",
                json_err.line, json_err.text);
        goto planLoadFromJsonFile_err;
    }

    //loadJsonData(plan, json, "use_metric", loadJsonUseMetric);
    if (loadJsonData(plan, json, "version", loadJsonVersion) != 0)
        goto planLoadFromJsonFile_err;
    //loadJsonData(plan, json, "use_metric", loadJsonUseMetric);
    if (loadJsonData(plan, json, "variable", loadJsonVariable) != 0)
        goto planLoadFromJsonFile_err;
    if (loadJsonData(plan, json, "initial_state", loadJsonInitialState) != 0)
        goto planLoadFromJsonFile_err;
    if (loadJsonData(plan, json, "goal", loadJsonGoal) != 0)
        goto planLoadFromJsonFile_err;
    if (loadJsonData(plan, json, "operator", loadJsonOperator) != 0)
        goto planLoadFromJsonFile_err;
    //loadJsonData(plan, json, "mutex_group", loadJsonMutexGroup);
    //loadJsonData(plan, json, "successor_generator", loadJsonSuccessorGenerator);
    //loadJsonData(plan, json, "axiom", loadJsonAxiom);
    //loadJsonData(plan, json, "causal_graph", loadJsonCausalGraph);
    //loadJsonData(plan, json, "domain_transition_graph",
    //             loadJsonDomainTransitionGrah);

    jsuccgen = json_object_get(json, "successor_generator");
    plan->succ_gen = planSuccGenFromJson(jsuccgen, plan->op);

    json_decref(json);
    return 0;

planLoadFromJsonFile_err:
    json_decref(json);
    return -1;
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
    ssize_t fact_name_len;
    int layer, range;

    if (fdAssert(fin, "begin_variable") != 0)
        return -1;
    if (fscanf(fin, "%s %d %d", sval, &layer, &range) != 3)
        return -1;

    var->name = strdup(sval);
    var->axiom_layer = layer;
    var->range = range;

    if ((int)var->range != range){
        fprintf(stderr, "Error: Could not load variable %s, because the\n"
                        "range can't be stored in %d bytes long variable.\n"
                        "Change definition of plan_val_t and recompile.\n",
                        var->name, (int)sizeof(var->range));
        return -1;
    }

    var->fact_name = BOR_ALLOC_ARR(char *, var->range);
    size = 0;
    fact_name = NULL;
    getline(&fact_name, &size, fin);
    for (i = 0; i < var->range; ++i){
        fact_name_len = getline(&fact_name, &size, fin);
        if (fact_name_len > 1){
            fact_name[fact_name_len - 1] = 0x0;
            var->fact_name[i] = strdup(fact_name);
        }else{
            var->fact_name[i] = strdup("");
        }
    }

    if (fact_name != NULL)
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

    planStateDel(plan->state_pool, state);

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
        planPartStateSet(plan->state_pool, plan->goal, var, val);
    }

    if (fdAssert(fin, "end_goal") != 0)
        return -1;

    return 0;
}

static int fdOperator(plan_operator_t *op, FILE *fin, int use_metric)
{
    char *name;
    size_t name_size;
    ssize_t name_len;
    int i, len, var, cond, pre, post, cost;

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
        planOperatorSetPrecondition(op, var, pre);
    }

    // pre-post
    if (fscanf(fin, "%d", &len) != 1)
        return -1;
    for (i = 0; i < len; ++i){
        if (fscanf(fin, "%d", &cond) != 1)
            return -1;
        if (cond > 0){
            fprintf(stderr, "Error: Don't know how to handle operator"
                            " effect precondition.\n");
            return -1;
        }

        if (fscanf(fin, "%d %d %d", &var, &pre, &post) != 3)
            return -1;

        if (pre != -1)
            planOperatorSetPrecondition(op, var, pre);
        planOperatorSetEffect(op, var, post);
    }

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
    plan->op = BOR_ALLOC_ARR(plan_operator_t, plan->op_size);
    for (i = 0; i < plan->op_size; ++i){
        planOperatorInit(plan->op + i, plan->state_pool);
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
    planStateDel(src->state_pool, state);

    dst->goal = planPartStateNew(dst->state_pool);
    PLAN_PART_STATE_FOR_EACH(src->goal, i, var, val){
        planPartStateSet(dst->state_pool, dst->goal, var, val);
    }

    dst->op_size = 0;
    dst->op = NULL;
    dst->succ_gen = NULL;
}

static int agentLoadOperators(plan_problem_t *prob,
                              const plan_operator_t *src_op,
                              FILE *fin, int private)
{
    int i, op_size, op_id, ins_from;

    ins_from = prob->op_size;

    if (fscanf(fin, "%d", &op_size) != 1)
        return -1;
    prob->op_size += op_size;
    prob->op = BOR_REALLOC_ARR(prob->op, plan_operator_t, prob->op_size);

    for (i = 0; i < op_size; ++i){
        if (fscanf(fin, "%d", &op_id) != 1)
            return -1;

        planOperatorInit(prob->op + ins_from, prob->state_pool);
        planOperatorCopy(prob->op + ins_from, src_op + op_id);
        if (private)
            planOperatorSetPrivate(prob->op + ins_from);
        ++ins_from;
    }

    return 0;
}

static int fdAgents(const plan_problem_t *prob, FILE *fin,
                    plan_problem_agent_t **agents_out, int *num_agents_out)
{
    plan_problem_agent_t *agents;
    plan_problem_agent_t *agent;
    int i, opi, num_agents, owner;
    char name[1024];

    if (fscanf(fin, "%d", &num_agents) != 1)
        return 0;

    agents = BOR_ALLOC_ARR(plan_problem_agent_t, num_agents);

    for (i = 0; i < num_agents; ++i){
        if (fdAssert(fin, "begin_agent") != 0)
            return -1;


        agent = agents + i;
        agentInitProblem(&agent->prob, prob);

        if (fscanf(fin, "%s %d", name, &agents[i].id) != 2)
            return -1;
        agents[i].name = strdup(name);

        if (fdAssert(fin, "begin_agent_private_operators") != 0)
            return -1;
        agentLoadOperators(&agent->prob, prob->op, fin, 1);
        if (fdAssert(fin, "end_agent_private_operators") != 0)
            return -1;

        if (fdAssert(fin, "begin_agent_public_operators") != 0)
            return -1;
        agentLoadOperators(&agent->prob, prob->op, fin, 0);
        if (fdAssert(fin, "end_agent_public_operators") != 0)
            return -1;

        agent->prob.succ_gen = planSuccGenNew(agent->prob.op,
                                              agent->prob.op_size);


        if (fdAssert(fin, "begin_agent_projected_operators") != 0)
            return -1;

        if (fscanf(fin, "%d", &agent->projected_op_size) != 1)
            return -1;
        agent->projected_op = BOR_ALLOC_ARR(plan_operator_t,
                                            agent->projected_op_size);
        for (opi = 0; opi < agent->projected_op_size; ++opi){
            planOperatorInit(agent->projected_op + opi, agent->prob.state_pool);
            fdOperator(agent->projected_op + opi, fin, 1);

            if (fscanf(fin, "%d", &owner) != 1)
                return -1;
            planOperatorSetOwner(agent->projected_op + opi, owner);
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

static int loadFDBase(plan_problem_t *plan, FILE *fin)
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

    if (loadFDBase(plan, fin) != 0)
        return -1;

    fclose(fin);

    return 0;
}

static int loadAgentFD(const char *filename,
                       plan_problem_agent_t **agents,
                       int *num_agents)
{
    FILE *fin;
    plan_problem_t prob;

    fin = fopen(filename, "r");
    if (fin == NULL){
        fprintf(stderr, "Error: Could not read `%s'.\n", filename);
        return -1;
    }

    planProblemInit(&prob);

    if (loadFDBase(&prob, fin) != 0)
        return -1;

    if (fdDomainTransitionGraph(&prob, fin) != 0)
        return -1;
    if (fdCausalGraph(&prob, fin) != 0)
        return -1;
    if (fdAgents(&prob, fin, agents, num_agents) != 0)
        return -1;

    planProblemFree(&prob);

    fclose(fin);

    if (*num_agents == 0)
        return -1;

    return 0;
}



void planProblemDump(const plan_problem_t *p, FILE *fout)
{
    int i, j;
    plan_state_t *state;

    state = planStateNew(p->state_pool);

    for (i = 0; i < p->var_size; ++i){
        fprintf(fout, "Var[%d] range: %d, name: %s\n",
                i, (int)p->var[i].range, p->var[i].name);
    }

    planStatePoolGetState(p->state_pool, p->initial_state, state);
    fprintf(fout, "Init:");
    for (i = 0; i < p->var_size; ++i){
        fprintf(fout, " %d:%d", i, (int)planStateGet(state, i));
    }
    fprintf(fout, "\n");

    fprintf(fout, "Goal:");
    for (i = 0; i < p->var_size; ++i){
        if (planPartStateIsSet(p->goal, i)){
            fprintf(fout, " %d:%d", i, (int)planPartStateGet(p->goal, i));
        }else{
            fprintf(fout, " %d:X", i);
        }
    }
    fprintf(fout, "\n");

    for (i = 0; i < p->op_size; ++i){
        fprintf(fout, "Op[%3d] %s\n", i, p->op[i].name);
        fprintf(fout, "  Pre: ");
        for (j = 0; j < p->var_size; ++j){
            if (planPartStateIsSet(p->op[i].pre, j)){
                fprintf(fout, " %d:%d", j, (int)planPartStateGet(p->op[i].pre, j));
            }else{
                fprintf(fout, " %d:X", j);
            }
        }
        fprintf(fout, "\n");
        fprintf(fout, "  Post:");
        for (j = 0; j < p->var_size; ++j){
            if (planPartStateIsSet(p->op[i].eff, j)){
                fprintf(fout, " %d:%u", j, (int)planPartStateGet(p->op[i].eff, j));
            }else{
                fprintf(fout, " %d:X", j);
            }
        }
        fprintf(fout, "\n");
    }

    planStateDel(p->state_pool, state);
}
