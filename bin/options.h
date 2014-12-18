#ifndef OPTIONS_H
#define OPTIONS_H

struct _options_t {
    int help;
    char *proto;
    char *output;
    int max_time;
    int max_mem;
    int progress_freq;
    int print_heur_init;
    char *dot_graph;

    char *heur;
    char **heur_opts;
    int heur_opts_len;

    char *search;
    char **search_opts;
    int search_opts_len;
};
typedef struct _options_t options_t;

int options(options_t *o, int argc, char *argv[]);
void optionsFree(options_t *o);
int optionsSearchOpt(const options_t *o, const char *optname);

#endif /* OPTIONS_H */
