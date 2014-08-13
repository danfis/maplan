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
};
typedef struct _graph_node_t graph_node_t;

/** Destroys graph */
static void graphDel(bor_rbtree_int_t *graph);
/** Adds one connection to the graph (or increases edge's value by one if
 *  the connection is already there). */
static void graphAddConnection(bor_rbtree_int_t *graph, int from, int to);
/** Creates a weighted graph from operators */
static void fillGraphs(plan_causal_graph_t *cg,
                       const plan_operator_t *op, int op_size);

plan_causal_graph_t *planCausalGraphNew(const plan_var_t *var, int var_size,
                                        const plan_operator_t *op, int op_size,
                                        const plan_part_state_t *goal)
{
    plan_causal_graph_t *cg;

    cg = BOR_ALLOC(plan_causal_graph_t);
    cg->successor_graph = borRBTreeIntNew();
    cg->predecessor_graph = borRBTreeIntNew();
    fillGraphs(cg, op, op_size);

    return cg;
}

void planCausalGraphDel(plan_causal_graph_t *cg)
{
    if (cg->successor_graph)
        graphDel(cg->successor_graph);
    if (cg->predecessor_graph)
        graphDel(cg->predecessor_graph);
    BOR_FREE(cg);
}

static void graphDel(bor_rbtree_int_t *graph)
{
    bor_rbtree_int_node_t *tree_node, *tree_edge;
    graph_node_t *node;
    graph_edge_t *edge;

    while (!borRBTreeIntEmpty(graph)){
        tree_node = borRBTreeIntExtractMin(graph);
        node = bor_container_of(tree_node, graph_node_t, rbtree);

        while (!borRBTreeIntEmpty(node->edge)){
            tree_edge = borRBTreeIntExtractMin(node->edge);
            edge = bor_container_of(tree_edge, graph_edge_t, rbtree);
            BOR_FREE(edge);
        }

        BOR_FREE(node);
    }

    borRBTreeIntDel(graph);
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
        borRBTreeIntInsert(graph, from, &node->rbtree);
    }

    tree_edge = borRBTreeIntFind(node->edge, to);
    if (tree_edge != NULL){
        edge = bor_container_of(tree_edge, graph_edge_t, rbtree);
    }else{
        edge = BOR_ALLOC(graph_edge_t);
        edge->value = 0;
        borRBTreeIntInsert(node->edge, to, &edge->rbtree);
    }

    ++edge->value;
}

static void fillGraphs(plan_causal_graph_t *cg,
                       const plan_operator_t *op, int op_size)
{
    plan_operator_cond_eff_t *cond_eff;
    int i, j, prei, effi;
    plan_var_id_t pre_var, eff_var;

    for (i = 0; i < op_size; ++i){
        PLAN_PART_STATE_FOR_EACH_VAR(op[i].pre, prei, pre_var){
            PLAN_PART_STATE_FOR_EACH_VAR(op[i].eff, effi, eff_var){
                graphAddConnection(cg->successor_graph, pre_var, eff_var);
                graphAddConnection(cg->predecessor_graph, eff_var, pre_var);
            }
        }

        for (j = 0; j < op[i].cond_eff_size; ++j){
            cond_eff = op[i].cond_eff + j;

            PLAN_PART_STATE_FOR_EACH_VAR(op[i].pre, prei, pre_var){
                PLAN_PART_STATE_FOR_EACH_VAR(cond_eff->eff, effi, eff_var){
                    graphAddConnection(cg->successor_graph, pre_var, eff_var);
                    graphAddConnection(cg->predecessor_graph, eff_var, pre_var);
                }
            }

            PLAN_PART_STATE_FOR_EACH_VAR(cond_eff->pre, prei, pre_var){
                PLAN_PART_STATE_FOR_EACH_VAR(cond_eff->eff, effi, eff_var){
                    graphAddConnection(cg->successor_graph, pre_var, eff_var);
                    graphAddConnection(cg->predecessor_graph, eff_var, pre_var);
                }
            }
        }
    }

}
