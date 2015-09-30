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

#include <boruvka/timer.h>
#include <plan/pddl_sas.h>

int main(int argc, char *argv[])
{
    plan_pddl_t *pddl;
    plan_pddl_ground_t ground;
    plan_pddl_sas_t sas;
    unsigned sas_flags = PLAN_PDDL_SAS_USE_CG;
    bor_timer_t timer;

    if (argc != 3){
        fprintf(stderr, "Usage: %s domain.pddl problem.pddl\n", argv[0]);
        return -1;
    }

    borTimerStart(&timer);

    pddl = planPDDLNew(argv[1], argv[2]);
    if (pddl == NULL){
        fprintf(stderr, "Error: Could not load PDDLs `%s' and `%s'.\n",
                argv[1], argv[2]);
        return -1;
    }

    planPDDLGroundInit(&ground, pddl);
    planPDDLGround(&ground);
    planPDDLGroundPrint(&ground, stdout);

    planPDDLSasInit(&sas, &ground);
    planPDDLSas(&sas, sas_flags);
    planPDDLSasPrintFacts(&sas, &ground, stdout);
    planPDDLSasPrintInvariant(&sas, &ground, stdout);
    planPDDLSasPrintInvariantFD(&sas, &ground, stdout);
    planPDDLSasFree(&sas);

    planPDDLGroundFree(&ground);
    planPDDLDel(pddl);

    borTimerStop(&timer);
    fprintf(stderr, "Elapsed Time: %.2fs\n",
            borTimerElapsedInSF(&timer));
    return 0;
}
