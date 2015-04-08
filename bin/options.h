#ifndef OPTIONS_H
#define OPTIONS_H

struct _options_t {
    int help;
    int ma_unfactor;
    int ma_factor;
    char *proto;
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
    int op_unit_cost;

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
