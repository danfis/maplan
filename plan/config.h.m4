#ifndef __PLAN_CONFIG_H__
#define __PLAN_CONFIG_H__

ifdef(`DEBUG', `#define PLAN_DEBUG')
ifdef(`USE_NANOMSG', `#define PLAN_NANOMSG')
ifdef(`USE_CPLEX', `#define PLAN_USE_CPLEX')
ifdef(`USE_LP_SOLVE', `#define PLAN_USE_LP_SOLVE')
ifdef(`LP', `#define PLAN_LP')

#endif /* __PLAN_CONFIG_H__ */
