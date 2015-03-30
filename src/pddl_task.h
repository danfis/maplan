#ifndef __PLAN_PDDL_TASK_H__
#define __PLAN_PDDL_TASK_H__

#include <stdio.h>
#include <boruvka/list.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_pddl_task_t {
    char *domain_name;
    char *problem_name;
    // types
    // objects
    // predicates
    // functions
    // init
    // goal
    // actions
    // metric
};
typedef struct _plan_pddl_task_t plan_pddl_task_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_TASK_H__ */
