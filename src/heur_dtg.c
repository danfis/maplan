#include <boruvka/alloc.h>
#include <plan/heur.h>

struct _plan_heur_dtg_t {
    plan_heur_t heur;
};
typedef struct _plan_heur_dtg_t plan_heur_dtg_t;

#define HEUR(parent) \
    bor_container_of((parent), plan_heur_dtg_t, heur)

static void heurDTGDel(plan_heur_t *_heur);
static void heurDTG(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res);

plan_heur_t *planHeurDTG(const plan_var_t *var, int var_size,
                         const plan_part_state_t *goal,
                         const plan_op_t *op, int op_size)
{
    plan_heur_dtg_t *hdtg;

    hdtg = BOR_ALLOC(plan_heur_dtg_t);
    _planHeurInit(&hdtg->heur, heurDTGDel, heurDTG);

    return &hdtg->heur;
}

static void heurDTGDel(plan_heur_t *_heur)
{
    plan_heur_dtg_t *hdtg = HEUR(_heur);
    _planHeurFree(&hdtg->heur);
    BOR_FREE(hdtg);
}

static void heurDTG(plan_heur_t *_heur, const plan_state_t *state,
                    plan_heur_res_t *res)
{
    res->heur = PLAN_HEUR_DEAD_END;
}
