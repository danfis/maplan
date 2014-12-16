#if !defined(PLAN_HEUR_RELAX_EXPLORE_CHECK_GOAL)
# error "Undefined PLAN_HEUR_RELAX_EXPLORE_CHECK_GOAL macro!"
#endif

#if !defined(PLAN_HEUR_RELAX_EXPLORE_OP_ADD)
# error "Undefined PLAN_HEUR_RELAX_EXPLORE_OP_ADD macro!"
#endif

{
    plan_prio_queue_t queue;
    int i, size, *op, op_id;
    int fact_id;
    plan_cost_t value;
    plan_heur_relax_fact_t *fact;

    relaxInit(relax);
    planPrioQueueInit(&queue);

    relaxAddInitState(relax, &queue, state);
    while (!planPrioQueueEmpty(&queue)){
        fact_id = planPrioQueuePop(&queue, &value);
        fact = relax->fact + fact_id;
        if (fact->value != value)
            continue;

        PLAN_HEUR_RELAX_EXPLORE_CHECK_GOAL;

        size = relax->cref.fact_pre[fact_id].size;
        op   = relax->cref.fact_pre[fact_id].op;
        for (i = 0; i < size; ++i){
            op_id = op[i];
            PLAN_HEUR_RELAX_EXPLORE_OP_ADD;
        }
    }

    planPrioQueueFree(&queue);
}

#undef PLAN_HEUR_RELAX_EXPLORE_CHECK_GOAL
#undef PLAN_HEUR_RELAX_EXPLORE_OP_ADD
