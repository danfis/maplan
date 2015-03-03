#include <strings.h>
#include <string.h>
#include <boruvka/alloc.h>
#include <opts.h>

#include "options.h"

struct _optdef_t {
    const char *name;
    const char **opts;
};
typedef struct _optdef_t optdef_t;

static char default_heur[] = "lm-cut";
static char default_search[] = "astar";

static const char *opt_search_ehc[] = {
    "pref", "pref_only", NULL
};
static const char *opt_search_lazy[] = {
    "pref", "pref_only", "list-bucket", "list-heap", "list-rb",
    "list-splay", NULL
};
static const char *opt_search_astar[] = {
    "pathmax", NULL
};
static const char *opt_empty[] = { NULL };
static const char *opt_heur_all[] = {
    "proj", "loc", "glob", NULL
};

static optdef_t opt_search[] = {
    { "ehc", opt_search_ehc },
    { "lazy", opt_search_lazy },
    { "astar", opt_search_astar },
};
static int opt_search_size = sizeof(opt_search) / sizeof(optdef_t);

static optdef_t opt_heur[] = {
    { "goalcount", opt_empty },
    { "add", opt_heur_all },
    { "max", opt_heur_all },
    { "ff", opt_heur_all },
    { "dtg", opt_heur_all },
    { "lm-cut", opt_heur_all },
    { "ma-max", opt_empty },
    { "ma-ff", opt_empty },
    { "ma-lm-cut", opt_empty },
    { "ma-dtg", opt_empty },
};
static int opt_heur_size = sizeof(opt_heur) / sizeof(optdef_t);

static int readOpts(options_t *o, int argc, char *argv[])
{
    int i;

    optsAddDesc("help", 'h', OPTS_NONE, &o->help, NULL,
                "Print this help.");
    optsAddDesc("ma", 0x0, OPTS_NONE, &o->ma, NULL,
                "Switch to multi-agent mode.");
    optsAddDesc("problem", 'p', OPTS_STR, &o->proto, NULL,
                "Path to a problem definition in .proto format.");
    optsAddDesc("search", 's', OPTS_STR, &o->search, NULL,
                "Define search algorithm. See below for options. (default: astar)");
    optsAddDesc("heur", 'H', OPTS_STR, &o->heur, NULL,
                "Define heuristic See below for options. (default: lm-cut)");
    optsAddDesc("output", 'o', OPTS_STR, &o->output, NULL,
                "Path where to write resulting plan. (default: None)");
    optsAddDesc("print-heur-init", 0x0, OPTS_NONE, &o->print_heur_init, NULL,
                "Prints heuristic value for the initial state to stdout."
                " (default: Off)");
    optsAddDesc("max-time", 0x0, OPTS_INT, &o->max_time, NULL,
                "Maximal time the search can spent on finding solution in"
                " seconds. (default: 30 minutes).");
    optsAddDesc("max-mem", 0x0, OPTS_INT, &o->max_mem, NULL,
                "Maximal memory (peak memory) in MB. (default: 1GB)");
    optsAddDesc("progress-freq", 0x0, OPTS_INT, &o->progress_freq, NULL,
                "Frequency in which progress bar is called."
                " (default: 10000)");
    optsAddDesc("dot-graph", 0x0, OPTS_STR, &o->dot_graph, NULL,
                "Prints problem definition as graph in DOT format in"
                " specified file. (default: None)");
    optsAddDesc("hard-limit-sleeptime", 0x0, OPTS_INT, &o->hard_limit_sleeptime,
                NULL, "Sleeptime in seconds for hard limit monitor."
                " Set to -1 to disable hard limit monitor. (default: 5)");
    optsAddDesc("op-unit-cost", 0x0, OPTS_NONE, &o->op_unit_cost, NULL,
                "Force costs of all operators to one. (default: off)");

    if (opts(&argc, argv) != 0){
        return -1;
    }
    if (argc != 1){
        fprintf(stderr, "Unrecognized options:");
        for (i = 1; i < argc; ++i)
            fprintf(stderr, " `%s'", argv[i]);
        fprintf(stderr, "\n");
        return -1;
    }

    return 0;
}

static void usage(const char *progname)
{
    fprintf(stderr, "Usage: %s [OPTIONS]\n", progname);
    fprintf(stderr, "  OPTIONS:\n");
    optsPrint(stderr, "    ");
    fprintf(stderr, "\n");
    fprintf(stderr,
"  SEARCH OPTIONS:\n"
"    Search option should be a string consisting of one or more options\n"
"    delimited by a semicolon. The first part must be name of the search\n"
"    followed by a list of options.\n"
"    The available search methods are: ehc, lazy, astar\n"
"\n"
"    Options allowed for *ehc*:\n"
"           pref      -- preferred operators are used\n"
"           pref_only -- only the preferred operators are used\n"
"\n"
"    Options allowed for *lazy*:\n"
"           pref        -- preferred operators are used\n"
"           pref_only   -- only the preferred operators are used\n"
"           list-bucket -- bucket based open-list is used\n"
"           list-heap   -- pairing heap based open-list\n"
"           list-rb     -- rb-tree based open-list\n"
"           list-splay  -- splay-tree based open-list (default)\n"
"\n"
"    Options allowed for *astar*:\n"
"           pathmax -- pathmax variant of A*\n"
"\n"
"    EXAMPLES:\n"
"           ehc:pref -- EHC algorithm with preferred operators\n"
"           lazy:pref:list-bucket -- Lazy algorithm with preferred\n"
"                                    operators and bucket based\n"
"                                    open-list\n"
);
    fprintf(stderr, "\n");
    fprintf(stderr,
"  HEUR OPTIONS:\n"
"    The available heur algorithms are: goalcount, add, max, ff, dtg, lm-cut.\n"
"    Additionally for the multi-agent mode: ma-max, ma-ff, ma-lm-cut, ma-dtg\n"
"\n"
"    Options allowed for all (non ma-) heuristics in multi-agent mode:\n"
"           proj -- heur is computed on projected operators\n"
"           loc  -- local operators (i.e., operators visible to an agent)\n"
"           glob -- global operators (i.e., all operators regardless which\n"
"                   agent own them)\n"
);
    fprintf(stderr, "\n");
}

static void splitOptList(char *in, char ***list, int *list_len)
{
    int i;
    int num_delim = 0;
    char *c;

    for (c = in; c && *c; ++c)
        num_delim += (*c == ':');

    if (num_delim == 0)
        return;

    *list_len = num_delim;
    *list = BOR_ALLOC_ARR(char *, num_delim);

    for (c = in; *c != ':'; ++c);
    *c = 0x0;

    i = 0;
    (*list)[i++] = ++c;
    for (; *c; ++c){
        if (*c == ':'){
            *c = 0x0;
            (*list)[i++] = (c + 1);
        }
    }
}

static int checkOptions(const optdef_t *def, int def_size,
                        const char *name, char **opts, int opts_len)
{
    int di, i, found, ok = 1;
    const char **l;

    for (di = 0; di < def_size; ++di){
        if (strcmp(def[di].name, name) == 0)
            break;
    }
    if (di == def_size){
        fprintf(stderr, "Error: Unrecognized method name: `%s'\n\n", name);
        return -1;
    }

    for (i = 0; i < opts_len; ++i){
        found = 0;
        for (l = def[di].opts; *l != NULL; ++l){
            if (strcmp(opts[i], *l) == 0){
                found = 1;
                break;
            }
        }

        if (found == 0){
            ok = 0;
            fprintf(stderr, "Error: Unrecognized option `%s' in this"
                            " context\n", opts[i]);
        }
    }

    if (!ok)
        return -1;
    return 0;
}

static int parseSearch(options_t *o)
{
    splitOptList(o->search, &o->search_opts, &o->search_opts_len);
    splitOptList(o->heur, &o->heur_opts, &o->heur_opts_len);

    if (checkOptions(opt_search, opt_search_size, o->search,
                     o->search_opts, o->search_opts_len) != 0)
        return -1;
    if (checkOptions(opt_heur, opt_heur_size, o->heur,
                     o->heur_opts, o->heur_opts_len) != 0)
        return -1;
    return 0;
}

static void printOpts(const options_t *o)
{
    int i;

    printf("Multi-agent: %d\n", o->ma);
    printf("Proto: %s\n", o->proto);
    printf("Output: %s\n", o->output);
    printf("Max time: %d s\n", o->max_time);
    printf("Max mem: %d MB\n", o->max_mem);
    printf("Progress freq: %d\n", o->progress_freq);
    printf("Print heur init: %d\n", o->print_heur_init);
    printf("Dot graph: %s\n", o->dot_graph);
    printf("Op unit cost: %d\n", o->op_unit_cost);
    printf("Heur: %s [", o->heur);
    for (i = 0; i < o->heur_opts_len; ++i){
        if (i > 0)
            printf(" ");
        printf("%s", o->heur_opts[i]);
    }
    printf("]\n");
    printf("Search: %s [", o->search);
    for (i = 0; i < o->search_opts_len; ++i){
        if (i > 0)
            printf(" ");
        printf("%s", o->search_opts[i]);
    }
    printf("]\n");
    printf("\n");
}

int options(options_t *o, int argc, char *argv[])
{
    bzero(o, sizeof(*o));
    o->max_time = 30 * 60;
    o->max_mem = 1024;
    o->progress_freq = 10000;
    o->heur = default_heur;
    o->heur_opts = NULL;
    o->heur_opts_len = 0;
    o->search = default_search;
    o->search_opts = NULL;
    o->search_opts_len = 0;
    o->hard_limit_sleeptime = 5;
    o->op_unit_cost = 0;

    if (readOpts(o, argc, argv) != 0 || o->help){
        usage(argv[0]);
        return -1;
    }

    if (o->proto == NULL){
        fprintf(stderr, "Error: Problem file not specified! (see -p"
                        " option)\n\n");
        usage(argv[0]);
        return -1;
    }

    if (parseSearch(o) != 0){
        usage(argv[0]);
        return -1;
    }

    printOpts(o);

    return 0;
}

void optionsFree(options_t *o)
{
    if (o->heur_opts)
        BOR_FREE(o->heur_opts);
    if (o->search_opts)
        BOR_FREE(o->search_opts);

    optsClear();
}

int optionsSearchOpt(const options_t *o, const char *optname)
{
    int i;
    for (i = 0; i < o->search_opts_len; ++i){
        if (strcmp(o->search_opts[i], optname) == 0)
            return 1;
    }
    return 0;
}

int optionsHeurOpt(const options_t *o, const char *optname)
{
    int i;
    for (i = 0; i < o->heur_opts_len; ++i){
        if (strcmp(o->heur_opts[i], optname) == 0)
            return 1;
    }
    return 0;
}
