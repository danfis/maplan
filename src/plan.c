#include <string.h>
#include <jansson.h>

#include <boruvka/alloc.h>

#include "plan/plan.h"

#define READ_BUFSIZE 1024


static void planFree(plan_t *plan)
{
    size_t i;

    if (plan->goal){
        planStatePoolDestroyPartState(plan->state_pool, plan->goal);
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

plan_t *planNew(void)
{
    plan_t *plan;

    plan = BOR_ALLOC(plan_t);
    plan->var = NULL;
    plan->var_size = 0;
    plan->state_pool = NULL;
    plan->op = NULL;
    plan->op_size = 0;
    plan->goal = NULL;

    return plan;
}

void planDel(plan_t *plan)
{
    planFree(plan);
    BOR_FREE(plan);
}


static int loadJsonVersion(plan_t *plan, json_t *json)
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
    int i;

    data = json_object_get(json, "name");
    var->name = strdup(json_string_value(data));

    data = json_object_get(json, "range");
    var->range = json_integer_value(data);

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

static int loadJsonVariable(plan_t *plan, json_t *json)
{
    size_t i;
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

static int loadJsonInitialState(plan_t *plan, json_t *json)
{
    size_t i, len;
    json_t *json_val;
    unsigned val;
    plan_state_t *state;

    // check we have correct size of the state
    len = json_array_size(json);
    if (len != plan->var_size){
        fprintf(stderr, "Error: Invalid number of variables in "
                        "initial state.\n");
        return -1;
    }

    // allocate a new state
    state = planStatePoolCreateState(plan->state_pool);

    // set up state variable values
    json_array_foreach(json, i, json_val){
        val = json_integer_value(json_val);
        planStateSet(state, i, val);
    }

    // insert state into state pool
    plan->initial_state = planStatePoolInsert(plan->state_pool, state);

    // deallocate memory for temporary state
    planStatePoolDestroyState(plan->state_pool, state);

    return 0;
}

static int loadJsonGoal(plan_t *plan, json_t *json)
{
    const char *key;
    json_t *json_val;
    unsigned var, val;

    // allocate a new state
    plan->goal = planStatePoolCreatePartState(plan->state_pool);

    json_object_foreach(json, key, json_val){
        var = atoi(key);
        val = json_integer_value(json_val);
        planPartStateSet(plan->goal, var, val);
    }

    return 0;
}

static int loadJsonOperator1(plan_operator_t *op, json_t *json)
{
    const char *key;
    json_t *data, *jprepost, *jpre, *jpost;
    int var, pre, post;

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

    return 0;
}

static int loadJsonOperator(plan_t *plan, json_t *json)
{
    size_t i;
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

typedef int (*load_json_data_fn)(plan_t *plan, json_t *json);
static int loadJsonData(plan_t *plan, json_t *root,
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

int planLoadFromJsonFile(plan_t *plan, const char *filename)
{
    json_t *json;
    json_error_t json_err;

    // Clear plan_t structure
    planFree(plan);

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


    json_decref(json);
    return 0;

planLoadFromJsonFile_err:
    json_decref(json);
    planFree(plan);
    return -1;
}
