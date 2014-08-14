#include <boruvka/alloc.h>

#include "plan/causalgraph.h"

struct _graph_edge_t {
    bor_rbtree_int_node_t rbtree;
    int value;
};
typedef struct _graph_edge_t graph_edge_t;

struct _graph_node_t {
    bor_rbtree_int_node_t rbtree;
    bor_rbtree_int_t *edge;
    int edge_size;
};
typedef struct _graph_node_t graph_node_t;

/** One strongly connected component */
struct _scc_comp_t {
    int *var;     /*!< Variables in component */
    int var_size; /*!< Number of variables in component */
};
typedef struct _scc_comp_t scc_comp_t;

/** Strongly connected components */
struct _scc_t {
    scc_comp_t *comp; /*!< List of components */
    int comp_size;    /*!< Number of components */
};
typedef struct _scc_t scc_t;

static void graphInit(plan_causal_graph_graph_t *g, int var_size);
static void graphFree(plan_causal_graph_graph_t *g);
/** Adds one connection to the graph (or increases edge's value by one if
 *  the connection is already there). */
static void graphAddConnection(bor_rbtree_int_t *graph, int from, int to);
/** Creates a weighted graph from operators */
static void fillGraphs(plan_causal_graph_t *cg,
                       const plan_operator_t *op, int op_size);
/** Determines strongly connected components */
static scc_t *sccNew(const plan_causal_graph_graph_t *graph, int var_size);
static void sccDel(scc_t *);

plan_causal_graph_t *planCausalGraphNew(const plan_var_t *var, int var_size,
                                        const plan_operator_t *op, int op_size,
                                        const plan_part_state_t *goal)
{
    plan_causal_graph_t *cg;
    scc_t *scc;

    cg = BOR_ALLOC(plan_causal_graph_t);
    cg->var_size = var_size;
    graphInit(&cg->successor_graph, var_size);
    graphInit(&cg->predecessor_graph, var_size);
    cg->important_var = BOR_ALLOC_ARR(int, cg->var_size);
    cg->var_order = BOR_ALLOC_ARR(int, cg->var_size);

    // Fill successor and predecessor graphs by dependencies between
    // preconditions and effects of operators.
    fillGraphs(cg, op, op_size);

    scc = sccNew(&cg->successor_graph, var_size);
    sccDel(scc);

    return cg;
}

void planCausalGraphDel(plan_causal_graph_t *cg)
{
    graphFree(&cg->successor_graph);
    graphFree(&cg->predecessor_graph);
    if (cg->important_var)
        BOR_FREE(cg->important_var);
    if (cg->var_order)
        BOR_FREE(cg->var_order);
    BOR_FREE(cg);
}

static void graphInit(plan_causal_graph_graph_t *g, int var_size)
{
    g->var_size = var_size;
    g->edge_size = BOR_CALLOC_ARR(int, g->var_size);
    g->value     = BOR_CALLOC_ARR(int *, g->var_size);
    g->end_var   = BOR_CALLOC_ARR(int *, g->var_size);
}

static void graphFree(plan_causal_graph_graph_t *g)
{
    int i;

    for (i = 0; i < g->var_size; ++i){
        if (g->value[i])
            BOR_FREE(g->value[i]);
        if (g->end_var[i])
            BOR_FREE(g->end_var[i]);
    }

    BOR_FREE(g->edge_size);
    BOR_FREE(g->value);
    BOR_FREE(g->end_var);
}

static void rbtreeToGraph(bor_rbtree_int_t *rb,
                          plan_causal_graph_graph_t *graph)
{
    bor_rbtree_int_node_t *tree_node, *tree_edge;
    graph_node_t *node;
    graph_edge_t *edge;
    int var, i, edge_size, *value, *end_var;

    while (!borRBTreeIntEmpty(rb)){
        tree_node = borRBTreeIntExtractMin(rb);
        node = bor_container_of(tree_node, graph_node_t, rbtree);
        var = borRBTreeIntKey(tree_node);

        edge_size = graph->edge_size[var] = node->edge_size;
        if (edge_size > 0){
            value = graph->value[var] = BOR_ALLOC_ARR(int, edge_size);
            end_var = graph->end_var[var] = BOR_ALLOC_ARR(int, edge_size);
        }

        for (i = 0; !borRBTreeIntEmpty(node->edge); ++i){
            tree_edge = borRBTreeIntExtractMin(node->edge);
            edge = bor_container_of(tree_edge, graph_edge_t, rbtree);

            end_var[i] = borRBTreeIntKey(tree_edge);
            value[i]   = edge->value;

            BOR_FREE(edge);
        }

        BOR_FREE(node);
    }
}

static void graphAddConnection(bor_rbtree_int_t *graph, int from, int to)
{
    bor_rbtree_int_node_t *tree_node, *tree_edge;
    graph_node_t *node;
    graph_edge_t *edge;

    tree_node = borRBTreeIntFind(graph, from);
    if (tree_node != NULL){
        node = bor_container_of(tree_node, graph_node_t, rbtree);
    }else{
        node = BOR_ALLOC(graph_node_t);
        node->edge = borRBTreeIntNew();
        node->edge_size = 0;
        borRBTreeIntInsert(graph, from, &node->rbtree);
    }

    tree_edge = borRBTreeIntFind(node->edge, to);
    if (tree_edge != NULL){
        edge = bor_container_of(tree_edge, graph_edge_t, rbtree);
    }else{
        edge = BOR_ALLOC(graph_edge_t);
        edge->value = 0;
        borRBTreeIntInsert(node->edge, to, &edge->rbtree);
        ++node->edge_size;
    }

    ++edge->value;
}

static void fillGraphs(plan_causal_graph_t *cg,
                       const plan_operator_t *op, int op_size)
{
    plan_operator_cond_eff_t *cond_eff;
    int i, j, prei, effi;
    plan_var_id_t pre_var, eff_var;
    bor_rbtree_int_t *succ_graph, *pred_graph;

    succ_graph = borRBTreeIntNew();
    pred_graph = borRBTreeIntNew();

    for (i = 0; i < op_size; ++i){
        PLAN_PART_STATE_FOR_EACH_VAR(op[i].pre, prei, pre_var){
            PLAN_PART_STATE_FOR_EACH_VAR(op[i].eff, effi, eff_var){
                graphAddConnection(succ_graph, pre_var, eff_var);
                graphAddConnection(pred_graph, eff_var, pre_var);
            }
        }

        for (j = 0; j < op[i].cond_eff_size; ++j){
            cond_eff = op[i].cond_eff + j;

            PLAN_PART_STATE_FOR_EACH_VAR(op[i].pre, prei, pre_var){
                PLAN_PART_STATE_FOR_EACH_VAR(cond_eff->eff, effi, eff_var){
                    graphAddConnection(succ_graph, pre_var, eff_var);
                    graphAddConnection(pred_graph, eff_var, pre_var);
                }
            }

            PLAN_PART_STATE_FOR_EACH_VAR(cond_eff->pre, prei, pre_var){
                PLAN_PART_STATE_FOR_EACH_VAR(cond_eff->eff, effi, eff_var){
                    graphAddConnection(succ_graph, pre_var, eff_var);
                    graphAddConnection(pred_graph, eff_var, pre_var);
                }
            }
        }
    }

    rbtreeToGraph(succ_graph, &cg->successor_graph);
    rbtreeToGraph(pred_graph, &cg->predecessor_graph);

    borRBTreeIntDel(succ_graph);
    borRBTreeIntDel(pred_graph);
}

/** Context for DFS during computing SCC */
struct _scc_dfs_t {
    const plan_causal_graph_graph_t *graph;
    int cur_index;
    int *index;
    int *lowlink;
    int *in_stack;
    int *stack;
    int stack_size;
};
typedef struct _scc_dfs_t scc_dfs_t;

static int cmpInt(const void *a, const void *b)
{
    return (*(int *)a) - (*(int *)b);
}

static void sccTarjanStrongconnect(scc_t *scc, scc_dfs_t *dfs, int var)
{
    int i, len, *end_var, w;
    scc_comp_t *comp;

    dfs->index[var] = dfs->lowlink[var] = dfs->cur_index++;
    dfs->stack[dfs->stack_size++] = var;
    dfs->in_stack[var] = 1;

    len     = dfs->graph->edge_size[var];
    end_var = dfs->graph->end_var[var];
    for (i = 0; i < len; ++i){
        w = end_var[i];
        if (dfs->index[w] == -1){
            sccTarjanStrongconnect(scc, dfs, w);
            dfs->lowlink[var] = BOR_MIN(dfs->lowlink[var], dfs->lowlink[w]);
        }else if (dfs->in_stack[w]){
            dfs->lowlink[var] = BOR_MIN(dfs->lowlink[var], dfs->lowlink[w]);
        }
    }

    if (dfs->index[var] == dfs->lowlink[var]){
        // Find how deep unroll stack
        for (i = dfs->stack_size - 1; dfs->stack[i] != var; --i)
            dfs->in_stack[dfs->stack[i]] = 0;
        dfs->in_stack[dfs->stack[i]] = 0;

        // Create new component
        ++scc->comp_size;
        scc->comp = BOR_REALLOC_ARR(scc->comp, scc_comp_t, scc->comp_size);
        comp = scc->comp + scc->comp_size - 1;
        comp->var_size = dfs->stack_size - i;
        comp->var = BOR_ALLOC_ARR(int, comp->var_size);

        // Copy variable IDs from stack to the component
        memcpy(comp->var, dfs->stack + i, sizeof(int) * comp->var_size);
        if (comp->var_size > 1)
            qsort(comp->var, comp->var_size, sizeof(int), cmpInt);

        // Shrink stack
        dfs->stack_size = i;
    }
}

static void sccTarjan(scc_t *scc, const plan_causal_graph_graph_t *graph,
                      int var_size)
{
    scc_dfs_t dfs;
    int i, var;

    // Initialize structure for Tarjan's algorithm
    dfs.graph = graph;
    dfs.cur_index = 0;
    dfs.index    = BOR_ALLOC_ARR(int, 4 * var_size);
    dfs.lowlink  = dfs.index + var_size;
    dfs.in_stack = dfs.lowlink + var_size;
    dfs.stack    = dfs.in_stack + var_size;
    dfs.stack_size = 0;
    for (i = 0; i < var_size; ++i){
        dfs.index[i] = dfs.lowlink[i] = -1;
        dfs.in_stack[i] = 0;
    }

    for (var = 0; var < var_size; ++var){
        if (dfs.index[var] == -1)
            sccTarjanStrongconnect(scc, &dfs, var);
    }

    BOR_FREE(dfs.index);
}

static scc_t *sccNew(const plan_causal_graph_graph_t *graph, int var_size)
{
    scc_t *scc;

    scc = BOR_ALLOC(scc_t);
    scc->comp_size = 0;
    scc->comp = NULL;

    // Run Tarjan's algorithm for finding strongly connected components.
    sccTarjan(scc, graph, var_size);

    /* DEBUG:
    int i, j;
    for (i = 0; i < scc->comp_size; ++i){
        for (j = 0; j < scc->comp[i].var_size; ++j){
            fprintf(stderr, " %d", scc->comp[i].var[j]);
        }
        fprintf(stderr, "\n");
    }
    */

    return scc;
}

static void sccDel(scc_t *scc)
{
    int i;

    for (i = 0; i < scc->comp_size; ++i){
        if (scc->comp[i].var != NULL)
            BOR_FREE(scc->comp[i].var);
    }
    if (scc->comp)
        BOR_FREE(scc->comp);
    BOR_FREE(scc);
}
