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

#ifndef OPTIONS_H
#define OPTIONS_H

struct _options_t {
    int help;
    int ma_unfactor;
    int ma_factor;
    int ma_factor_dir;
    char *proto;
    char *fd;
    char *pddl_domain;
    char *pddl_problem;
    char *output;
    char **tcp;
    int tcp_size;
    int tcp_id;
    int max_time;
    int max_mem;
    int progress_freq;
    int print_heur_init;
    char *dot_graph;
    int hard_limit_sleeptime;

    char *heur;
    char **heur_opts;
    int heur_opts_len;

    char *search;
    char **search_opts;
    int search_opts_len;
};
typedef struct _options_t options_t;

options_t *options(int argc, char *argv[]);
void optionsFree(void);
int optionsSearchOpt(const options_t *o, const char *optname);
int optionsHeurOpt(const options_t *o, const char *optname);

#endif /* OPTIONS_H */
