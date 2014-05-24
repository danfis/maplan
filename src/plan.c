#include <string.h>
#include <jansson.h>

#include <boruvka/alloc.h>

#include "plan/plan.h"

#define READ_BUFSIZE 1024

plan_t *planNew(void)
{
    plan_t *plan;

    plan = BOR_ALLOC(plan_t);
    plan->var = NULL;
    plan->var_size = 0;

    return plan;
}

void planDel(plan_t *plan)
{
    size_t i;

    if (plan->var != NULL){
        for (i = 0; i < plan->var_size; ++i){
            planVarFree(plan->var + i);
        }
        BOR_FREE(plan->var);
    }

    BOR_FREE(plan);
}

static void loadVariable(plan_var_t *var, json_t *json)
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

#if 0
    fprintf(stderr, "%s: %d | %d\n", var->name, var->range, var->axiom_layer);
    for (i = 0; i < var->range; ++i){
        fprintf(stderr, " %s", var->fact_name[i]);
    }
    fprintf(stderr, "\n");
#endif
}

static void loadVariables(plan_t *plan, json_t *json)
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
        loadVariable(plan->var + i, json_var);
    }
}

void planLoadFromJsonFile(plan_t *plan, const char *filename)
{
    json_t *json;
    json_t *data;
    json_error_t json_err;

    // TODO: Clear plan_t structure

    // open JSON data from file
    json = json_load_file(filename, 0, &json_err);
    if (json == NULL){
        // TODO: Error logging
        fprintf(stderr, "Error: Could not read json data. Line %d: %s.\n",
                json_err.line, json_err.text);
        exit(-1);
    }

    data = json_object_get(json, "variable");
    if (data == NULL){
        fprintf(stderr, "Error: No 'variable' key in json definition.\n");
        exit(-1);
    }
    loadVariables(plan, data);


    // free json resources
    json_decref(json);

#if 0

    fin = fopen(filename, "r");
    if (fin == NULL){
        fprintf(stderr, "Could not open file %s\n", filename);
        exit(-1);
    }

    // Ignore version, just read it
    readVersion(fin);
    // TODO
    readMetric(fin);

    loadVars(fin, plan);

    fclose(fin);
#endif
}
