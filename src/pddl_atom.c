#include <boruvka/alloc.h>
#include "pddl_atom.h"

/** Initializes a single atom */
static void planPDDLAtomInit(plan_pddl_atom_t *atom, int id, const char *name);
/** Frees allocated resources */
static void planPDDLAtomFree(plan_pddl_atom_t *atom);

static void planPDDLAtomInit(plan_pddl_atom_t *atom, int id, const char *name)
{
    atom->id = id;
    atom->name = BOR_STRDUP(name);
    atom->arity = 0;
    atom->args = NULL;
}

static void planPDDLAtomFree(plan_pddl_atom_t *atom)
{
    if (atom->name)
        BOR_FREE(atom->name);
    if (atom->args)
        BOR_FREE(atom->args);
}


void planPDDLAtomAddArg(plan_pddl_atom_t *atom, int arg)
{
    ++atom->arity;
    atom->args = BOR_REALLOC_ARR(atom->args, int, atom->arity);
    atom->args[atom->arity - 1] = arg;
}

static int treeCmp(const bor_rbtree_node_t *n1,
                   const bor_rbtree_node_t *n2, void *data)
{
    const plan_pddl_atom_t *a1 = bor_container_of(n1, plan_pddl_atom_t, tree);
    const plan_pddl_atom_t *a2 = bor_container_of(n2, plan_pddl_atom_t, tree);
    return strcmp(a1->name, a2->name);
}

void planPDDLAtomsInit(plan_pddl_atoms_t *p)
{
    p->size = 0;
    p->alloc = 5;
    p->atom = BOR_ALLOC_ARR(plan_pddl_atom_t, p->alloc);
    borRBTreeInit(&p->tree, treeCmp, NULL);
}

void planPDDLAtomsFree(plan_pddl_atoms_t *p)
{
    int i;

    borRBTreeFree(&p->tree);
    for (i = 0; i < p->size; ++i)
        planPDDLAtomFree(p->atom + i);

    if (p->atom != NULL)
        BOR_FREE(p->atom);
}

plan_pddl_atom_t *planPDDLAtomsFind(const plan_pddl_atoms_t *p,
                                    const char *name)
{
    plan_pddl_atom_t atom;
    bor_rbtree_node_t *tnode;

    atom.name = (char *)name;
    tnode = borRBTreeFind((bor_rbtree_t *)&p->tree, &atom.tree);
    if (tnode == NULL)
        return NULL;

    return bor_container_of(tnode, plan_pddl_atom_t, tree);
}

plan_pddl_atom_t *planPDDLAtomsAdd(plan_pddl_atoms_t *p,
                                   const char *name)
{
    bor_rbtree_node_t *tnode;
    int id;

    if (p->size == p->alloc){
        p->alloc *= 2;
        p->atom = BOR_REALLOC_ARR(p->atom, plan_pddl_atom_t, p->alloc);
    }

    id = p->size++;
    planPDDLAtomInit(p->atom + id, id, name);

    tnode = borRBTreeInsert(&p->tree, &p->atom[id].tree);
    if (tnode == NULL)
        return p->atom + id;

    planPDDLAtomFree(p->atom + id);
    --p->size;
    return bor_container_of(tnode, plan_pddl_atom_t, tree);
}
