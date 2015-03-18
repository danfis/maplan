#! /usr/bin/env python

from __future__ import print_function

from collections import defaultdict

import build_model
import pddl_to_prolog
import pddl
import timers

def get_fluent_facts(task, model):
    fluent_predicates = set()
    for action in task.actions:
        for effect in action.effects:
            fluent_predicates.add(effect.literal.predicate)
    for axiom in task.axioms:
        fluent_predicates.add(axiom.name)
    return set([fact for fact in model
                if fact.predicate in fluent_predicates])

def get_objects_by_type(typed_objects, types):
    result = defaultdict(list)
    supertypes = {}
    for type in types:
        supertypes[type.name] = type.supertype_names
    for obj in typed_objects:
        result[obj.type].append(obj.name)
        for type in supertypes[obj.type]:
            result[type].append(obj.name)
    return result

def instantiate(task, model):
    relaxed_reachable = False
    fluent_facts = get_fluent_facts(task, model)
    init_facts = set(task.init)

    type_to_objects = get_objects_by_type(task.objects, task.types)

    instantiated_actions = []
    instantiated_axioms = []
    reachable_action_parameters = defaultdict(list)
    for atom in model:
        if isinstance(atom.predicate, pddl.Action):
            action = atom.predicate
            parameters = action.parameters
            inst_parameters = atom.args[:len(parameters)]
            # Note: It's important that we use the action object
            # itself as the key in reachable_action_parameters (rather
            # than action.name) since we can have multiple different
            # actions with the same name after normalization, and we
            # want to distinguish their instantiations.
            reachable_action_parameters[action].append(inst_parameters)
            variable_mapping = dict([(par.name, arg)
                                     for par, arg in zip(parameters, atom.args)])
            inst_action = action.instantiate(variable_mapping, init_facts,
                                             fluent_facts, type_to_objects)
            if inst_action:
                instantiated_actions.append(inst_action)
        elif isinstance(atom.predicate, pddl.Axiom):
            axiom = atom.predicate
            variable_mapping = dict([(par.name, arg)
                                     for par, arg in zip(axiom.parameters, atom.args)])
            inst_axiom = axiom.instantiate(variable_mapping, init_facts, fluent_facts)
            if inst_axiom:
                instantiated_axioms.append(inst_axiom)
        elif atom.predicate == "@goal-reachable":
            relaxed_reachable = True

    return (relaxed_reachable, fluent_facts, instantiated_actions,
            sorted(instantiated_axioms), reachable_action_parameters)

def _explore(task, add_fluents = set()):
    prog = pddl_to_prolog.translate(task, add_fluents)
    model = build_model.compute_model(prog)
    with timers.timing("Completing instantiation"):
        return instantiate(task, model)

def public_fluents(fluents):
    fluents = filter(lambda x: not x.is_private, fluents)
    return [(f.predicate, f.args) for f in fluents]

def _exploreMA(task, add_pub_atoms = set()):
    result = _explore(task, add_pub_atoms)
    task.mark_private_atoms(result[1])
    pub_fluents = public_fluents(result[1])
    return result, pub_fluents

def exploreMA(task, comm):
    add_atoms = set()

    if comm.is_master:
        # Initial exploration done by the master agent
        res, pub_fluents = _exploreMA(task, add_atoms)
        comm.sendInRing(pub_fluents)

    while True:
        # receive all public fluents from the previous agent in ring
        pub_fluents = comm.recvInRing()

        if pub_fluents is None:
            # Detect end of the distributed exploration
            if not comm.is_master:
                comm.sendInRing(None)
            break

        if comm.agent_id == 0:
            # if the master agent has received the same set of fluents as
            # it already has, it means that the set cannot change anymore
            pub_cmp = public_fluents(res[1])
            if sorted(pub_fluents) == sorted(pub_cmp):
                comm.sendInRing(None)
                continue

        add_atoms = set([pddl.Atom(x[0], x[1]) for x in pub_fluents])
        res, pub_fluents = _exploreMA(task, add_atoms)
        comm.sendInRing(pub_fluents)

    return res

def explore(task, comm = None):
    if comm is not None:
        return exploreMA(task, comm)
    else:
        return _explore(task)

if __name__ == "__main__":
    task = pddl.open()
    relaxed_reachable, atoms, actions, axioms, _ = explore(task)
    print("goal relaxed reachable: %s" % relaxed_reachable)
    print("%d atoms:" % len(atoms))
    for atom in atoms:
        print(" ", atom)
    print()
    print("%d actions:" % len(actions))
    for action in actions:
        action.dump()
        print()
    print()
    print("%d axioms:" % len(axioms))
    for axiom in axioms:
        axiom.dump()
        print()
