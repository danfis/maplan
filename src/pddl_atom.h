#ifndef __PLAN_PDDL_ATOM_H__
#define __PLAN_PDDL_ATOM_H__

#include <stdio.h>
#include <boruvka/rbtree.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _plan_pddl_atom_t {
    int id;
    char *name;
    int arity;
    int *args;
    bor_rbtree_node_t tree;
};
typedef struct _plan_pddl_atom_t plan_pddl_atom_t;

/**
 * Adds argument to the atom.
 */
void planPDDLAtomAddArg(plan_pddl_atom_t *atom, int arg);


struct _plan_pddl_atoms_t {
    int size;
    int alloc;
    plan_pddl_atom_t *atom;
    bor_rbtree_t tree;
};
typedef struct _plan_pddl_atoms_t plan_pddl_atoms_t;

/**
 * Initialize pool of atoms.
 */
void planPDDLAtomsInit(plan_pddl_atoms_t *p);

/**
 * Frees allocated resources.
 */
void planPDDLAtomsFree(plan_pddl_atoms_t *p);

/**
 * Find the specified atom or return NULL if not found.
 */
plan_pddl_atom_t *planPDDLAtomsFind(const plan_pddl_atoms_t *p,
                                    const char *name);

/**
 * Adds an atom of a specified name into pool of atoms if not already
 * there. In either case the pointer to the corresponding atom is returned.
 */
plan_pddl_atom_t *planPDDLAtomsAdd(plan_pddl_atoms_t *p,
                                   const char *name);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PLAN_PDDL_ATOM_H__ */
