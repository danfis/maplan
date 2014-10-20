#ifndef __PLAN_FACT_ID_H__
#define __PLAN_FACT_ID_H__

#include <plan/var.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Structure for translation of variable's value to fact id.
 */
struct _plan_fact_id_t {
    int **fact_id;
    int fact_size;
    int var_size;
};
typedef struct _plan_fact_id_t plan_fact_id_t;

/**
 * Initializes struct for translating variable value to fact ID
 */
void planFactIdInit(plan_fact_id_t *fid, const plan_var_t *var, int var_size);

/**
 * Frees val_to_id_t resources
 */
void planFactIdFree(plan_fact_id_t *factid);

/**
 * Translates variable value to fact ID
 */
_bor_inline int planFactId(const plan_fact_id_t *factid,
                           plan_var_id_t var, plan_val_t val);

/**** INLINES ****/

_bor_inline int planFactId(const plan_fact_id_t *fid,
                           plan_var_id_t var, plan_val_t val)
{
    return fid->fact_id[var][val];
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_FACT_ID_H__ */
