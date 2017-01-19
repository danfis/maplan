/***
 * maplan
 * -------
 * Copyright (c)2015 Daniel Fiser <danfis@danfis.cz>,
 * Agent Technology Center, Department of Computer Science,
 * Faculty of Electrical Engineering, Czech Technical University in Prague.
 * All rights reserved.
 *
 * This file is part of maplan.
 *
 * Distributed under the OSI-approved BSD License (the "License");
 * see accompanying file BDS-LICENSE for details or see
 * <http://www.opensource.org/licenses/bsd-license.php>.
 *
 * This software is distributed WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the License for more information.
 */

#if !defined(PLAN_HEUR_RELAX_EXPLORE_CHECK_GOAL)
# error "Undefined PLAN_HEUR_RELAX_EXPLORE_CHECK_GOAL macro!"
#endif

#if !defined(PLAN_HEUR_RELAX_EXPLORE_OP_ADD)
# error "Undefined PLAN_HEUR_RELAX_EXPLORE_OP_ADD macro!"
#endif

{
    plan_prio_queue_t queue;
    int fact_id, op_id;
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

        PLAN_ARR_INT_FOR_EACH(relax->cref.fact_pre + fact_id, op_id){
            PLAN_HEUR_RELAX_EXPLORE_OP_ADD;
        }
    }

    planPrioQueueFree(&queue);
}

#undef PLAN_HEUR_RELAX_EXPLORE_CHECK_GOAL
#undef PLAN_HEUR_RELAX_EXPLORE_OP_ADD
