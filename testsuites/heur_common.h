#ifndef TEST_HEUR_COMMON_H
#define TEST_HEUR_COMMON_H

#include "plan/heur.h"

typedef plan_heur_t *(*new_heur_fn)(plan_problem_t *p);
void runHeurTest(const char *name,
                 const char *proto, const char *states,
                 new_heur_fn new_heur,
                 int pref, int landmarks);
void runHeurAStarTest(const char *name, const char *proto,
                      new_heur_fn new_heur, int max_steps);

#endif
