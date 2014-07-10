#include <boruvka/timer.h>
#include <boruvka/alloc.h>
#include <boruvka/tasks.h>
#include <plan/search.h>
#include <plan/heur.h>
#include <plan/search.h>
#include <plan/ma_agent.h>

void taskRun(int id, void *_agent, const bor_tasks_thinfo_t *thinfo)
{
    plan_ma_agent_t *agent = _agent;
    planMAAgentRun(agent);
}


int main(int argc, char *argv[])
{
    plan_problem_agents_t *prob = NULL;
    plan_heur_t **heur;
    plan_search_t **search;
    plan_search_ehc_params_t ehc_params;
    plan_ma_comm_queue_pool_t *queue_pool;
    plan_ma_agent_t **agent;
    bor_tasks_t *tasks;
    bor_timer_t timer;
    int i, res;
    FILE *fout;

    if (argc != 2){
        fprintf(stderr, "Usage: %s prob.asas\n", argv[0]);
        return -1;
    }

    borTimerStart(&timer);

    prob = planProblemAgentsFromFD(argv[1]);
    if (prob == NULL || prob->agent_size <= 1){
        fprintf(stderr, "Error: More than one agent must be defined (got %d).",
                prob->agent_size);
        return -1;
    }

    heur   = BOR_ALLOC_ARR(plan_heur_t *, prob->agent_size);
    search = BOR_ALLOC_ARR(plan_search_t *, prob->agent_size);
    agent  = BOR_ALLOC_ARR(plan_ma_agent_t *, prob->agent_size);

    queue_pool = planMACommQueuePoolNew(prob->agent_size);

    for (i = 0; i < prob->agent_size; ++i){
        plan_problem_t *p = &prob->agent[i].prob;
        heur[i] = planHeurRelaxAddNew(p->var, p->var_size, p->goal,
                                      p->op, p->op_size, p->succ_gen);

        planSearchEHCParamsInit(&ehc_params);
        ehc_params.heur = heur[i];
        ehc_params.search.prob = &prob->agent[i].prob;
        search[i] = planSearchEHCNew(&ehc_params);

        agent[i] = planMAAgentNew(prob->agent + i, search[i],
                                  planMACommQueue(queue_pool, i));
    }

    tasks = borTasksNew(prob->agent_size);
    for (i = 0; i < prob->agent_size; ++i){
        borTasksAdd(tasks, taskRun, i, agent[i]);
    }
    borTasksRun(tasks);
    borTasksDel(tasks);

    for (i = 0; i < prob->agent_size; ++i){
        planMAAgentDel(agent[i]);
        planSearchDel(search[i]);
        planHeurDel(heur[i]);
    }

    planMACommQueuePoolDel(queue_pool);

    BOR_FREE(agent);
    BOR_FREE(search);
    BOR_FREE(heur);

    planProblemAgentsDel(prob);

    return 0;
}

