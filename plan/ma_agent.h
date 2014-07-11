#ifndef __PLAN_MA_AGENT_H__
#define __PLAN_MA_AGENT_H__

#include <plan/search.h>
#include <plan/ma_comm_queue.h>


/**
 * Operator in a found path.
 */
struct _plan_ma_agent_path_op_t {
    char *name;       /*!< Name of the operator */
    plan_cost_t cost; /*!< Cost of the operator */
};
typedef struct _plan_ma_agent_path_op_t plan_ma_agent_path_op_t;

struct _plan_ma_agent_t {
    plan_search_t *search;         /*!< Instance of search algorithm */
    plan_ma_comm_queue_t *comm;    /*!< Communication queue */
    plan_state_pool_t *state_pool; /*!< State pool from .prob */
    int pub_state_reg_id;          /*!< ID of the registry that associates
                                        received public state with state-id. */
    size_t packed_state_size;      /*!< Number of bytes required for packed
                                        state */
    plan_ma_agent_path_op_t *path; /*!< Path found by the agent */
    int path_size;                 /*!< Number of operators in the path */
    int found;                     /*!< True if solution was found */
    int terminated;                /*!< True if the agent was already
                                        terminated */
};
typedef struct _plan_ma_agent_t plan_ma_agent_t;

/**
 * Creates a multi-agent node.
 */
plan_ma_agent_t *planMAAgentNew(plan_search_t *search,
                                plan_ma_comm_queue_t *comm);

/**
 * Deletes multi-agent node.
 */
void planMAAgentDel(plan_ma_agent_t *agent);

/**
 * Runs multi-agent node in the current thread.
 */
int planMAAgentRun(plan_ma_agent_t *agent);

/**
 * Returns found path via output arguments. The memory is allocated on
 * heap.
 */
void planMAAgentGetPath(const plan_ma_agent_t *agent,
                        plan_ma_agent_path_op_t **path,
                        int *path_size);

/**
 * Frees allocated path
 */
void planMAAgentPathFree(plan_ma_agent_path_op_t *path, int path_size);

#endif /* __PLAN_MA_AGENT_H__ */
