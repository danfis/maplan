#include <plan/search.h>
#include <plan/heur.h>
#include <plan/search.h>
#include <opts.h>

static char *def_problem = NULL;
static char *def_search = "ehc";
static char *def_list = "heap";
static char *def_heur = "goalcount";
static int max_time = 60 * 30; // 30 minutes
static int max_mem = 1024 * 1024; // 1GB

static int readOpts(int argc, char *argv[])
{
    int help;

    optsAddDesc("help", 'h', OPTS_NONE, &help, NULL,
                "Print this help.");
    optsAddDesc("problem", 'p', OPTS_STR, &def_problem, NULL,
                "Path to the .json problem definition.");
    optsAddDesc("search", 's', OPTS_STR, &def_search, NULL,
                "Define search algorithm [ehc|lazy] (default: ehc)");
    optsAddDesc("list", 'l', OPTS_STR, &def_list, NULL,
                "Define list type [heap|bucket] (default: heap)");
    optsAddDesc("heur", 'H', OPTS_STR, &def_heur, NULL,
                "Define heuristic [goalcount|add|max|ff] (default: goalcount)");
    optsAddDesc("max-time", 0x0, OPTS_INT, &max_time, NULL,
                "Maximal time the search can spent on finding solution in"
                " seconds. (default: 30 minutes).");
    optsAddDesc("max-mem", 0x0, OPTS_INT, &max_mem, NULL,
                "Maximal memory (peak memory) in kb. (default: 1GB)");

    if (opts(&argc, argv) != 0){
        return -1;
    }

    if (help)
        return -1;

    if (def_problem == NULL){
        fprintf(stderr, "Error: Problem must be defined (-p).\n");
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
}

int main(int argc, char *argv[])
{
    plan_problem_t *prob;

    if (readOpts(argc, argv) != 0){
        usage(argv[0]);
        return -1;
    }

    printf("Problem: %s\n", def_problem);
    printf("Search: %s\n", def_search);
    printf("List: %s\n", def_list);
    printf("Heur: %s\n", def_heur);
    printf("Max Time: %d s\n", max_time);
    printf("Max Mem: %d kb\n", max_mem);
    printf("\n");

    prob = planProblemFromJson(def_problem);

    printf("Num variables: %d\n", prob->var_size);
    printf("Num operators: %d\n", prob->op_size);
    printf("Bytes per state: %d\n",
           planStatePackerBufSize(prob->state_pool->packer));
    printf("Size of state id: %d\n", (int)sizeof(plan_state_id_t));

    planProblemDel(prob);

    return 0;
}
