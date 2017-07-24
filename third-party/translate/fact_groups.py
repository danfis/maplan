from __future__ import print_function

import invariant_finder
import pddl
import timers
import sys


DEBUG = False


def expand_group(group, task, reachable_facts):
    result = []
    for fact in group:
        try:
            pos = list(fact.args).index("?X")
        except ValueError:
            result.append(fact)
        else:
            # NOTE: This could be optimized by only trying objects of the correct
            #       type, or by using a unifier which directly generates the
            #       applicable objects. It is not worth optimizing this at this stage,
            #       though.
            for obj in task.objects:
                newargs = list(fact.args)
                newargs[pos] = obj.name
                atom = pddl.Atom(fact.predicate, newargs)
                if atom in reachable_facts:
                    result.append(atom)
    task.mark_private_atoms(result)
    return result

def instantiate_groups(groups, task, reachable_facts):
    return [expand_group(group, task, reachable_facts) for group in groups]

class GroupCoverQueue:
    def __init__(self, groups, partial_encoding):
        self.partial_encoding = partial_encoding
        if groups:
            self.max_size = max([len(group) for group in groups])
            self.groups_by_size = [[] for i in range(self.max_size + 1)]
            self.groups_by_fact = {}
            for group in groups:
                group = set(group) # Copy group, as it will be modified.
                self.groups_by_size[len(group)].append(group)
                for fact in group:
                    self.groups_by_fact.setdefault(fact, []).append(group)
            self._update_top()
        else:
            self.max_size = 0
    def __bool__(self):
        return self.max_size > 1
    __nonzero__ = __bool__
    def pop(self):
        result = list(self.top) # Copy; this group will shrink further.
        if self.partial_encoding:
            for fact in result:
                for group in self.groups_by_fact[fact]:
                    group.remove(fact)
        self._update_top()
        return result
    def _update_top(self):
        while self.max_size > 1:
            max_list = self.groups_by_size[self.max_size]
            while max_list:
                candidate = max_list.pop()
                if len(candidate) == self.max_size:
                    self.top = candidate
                    return
                self.groups_by_size[len(candidate)].append(candidate)
            self.max_size -= 1

def choose_groups(groups, reachable_facts, partial_encoding=True):
    queue = GroupCoverQueue(groups, partial_encoding=partial_encoding)
    uncovered_facts = reachable_facts.copy()
    result = []
    while queue:
        group = queue.pop()
        uncovered_facts.difference_update(group)
        result.append(group)
    print(len(uncovered_facts), "uncovered facts")
    result += [[fact] for fact in uncovered_facts]
    return result

def build_translation_key(groups):
    group_keys = []
    for group in groups:
        group_key = [str(fact) for fact in group]
        if len(group) == 1:
            group_key.append(str(group[0].negate()))
        else:
            group_key.append("<none of those>")
        group_keys.append(group_key)
    return group_keys

def collect_all_mutex_groups(groups, atoms):
    # NOTE: This should be functionally identical to choose_groups
    # when partial_encoding is set to False. Maybe a future
    # refactoring could take that into account.
    all_groups = []
    uncovered_facts = atoms.copy()
    for group in groups:
        uncovered_facts.difference_update(group)
        all_groups.append(group)
    all_groups += [[fact] for fact in uncovered_facts]
    return all_groups

def sort_groups(groups):
    return sorted(sorted(group) for group in groups)

def public_groups(groups):
    groups = [[(x.predicate, x.args) for x in g if not x.is_private] for g in groups]
    groups = [x for x in groups if len(x) > 0]
    return groups

def ma_instantiate_groups(comm, groups, invariants, task, atoms):
    # Send public mutex groups to all other agents
    task.mark_private_atoms(groups)
    comm.sendToAll(public_groups(groups))

    # Extend set of initial atoms by mutex groups received from other
    # agents
    init_atoms = set(task.init)
    for other_groups in comm.recvFromAll():
        for group in other_groups:
            if len(group) <= 1:
                continue

            group = [pddl.Atom(x[0], x[1]) for x in group]
            if all([x not in init_atoms for x in group]):
                init_atoms.add(group[0])

    # If we haven't received any additional atom, exit with the old mutex
    # groups
    if len(init_atoms) == len(task.init):
        return groups

    # Try to instantiate mutex groups from a newly constructed initial
    # state
    invariant_groups = invariant_finder.useful_groups(invariants, init_atoms)
    invariant_groups = list(invariant_groups)
    new_groups = instantiate_groups(invariant_groups, task, atoms)
    if len(new_groups) > len(groups):
        return new_groups
    return groups

def split_groups_by_private_atoms(groups):
    result = []
    for group in groups:
        public = [x for x in group if not x.is_private]
        private = [x for x in group if x.is_private]
        if len(public) > 0:
            result += [public]
        if len(private) > 0:
            result += [private]
    return result

def unreachable_atoms(atoms, init, actions):
    reach = set(init)
    for a in actions:
        reach |= set(a.precondition)
        reach |= set([x[1] for x in a.add_effects])
        reach |= set([x[1] for x in a.del_effects])
#        print(a.name)
#        print('\t', a.precondition)
#        print('\t', a.add_effects)
#        print('\t', a.del_effects)
    unreach = set(atoms) - reach
    unreach = [x for x in unreach if str(x).startswith('Atom ')]
#    print('Reach:', reach)
#    print('Unreach:', unreach)
#    print('Unreach:', [x.is_private for x in unreach])
    return unreach

def extend_groups(groups, atoms, init, actions):
    unreach = unreachable_atoms(atoms, init, actions)
    print('Unreach:', sorted(unreach))
    for f in unreach:
        print('\t', f)
    return [set(g) | set(unreach) for g in groups]

MAX_GROUPS = 500
MAX_GROUPS = 5000000
class MutexGroups(object):
    def __init__(self, groups):
        self.groups = set([frozenset(g) for g in groups])

    def update(self, groups):
        print('Update: start with:', len(self.groups), len(groups))
        print('   -->', sorted(list(set([len(g) for g in self.groups]))))
        sys.stdout.flush()
        res = set()
        for g in groups:
            g = frozenset(g)
            for g1 in self.groups:
                x = g & g1
                if len(x) > 0:
                    l = len(res)
                    res.add(x)
                    if l != len(res):# and len(res) % 10000 == 0:
                        print('Add', len(res), end = '\r')
                        sys.stdout.flush()
        print()
        if len(res) > MAX_GROUPS:
            print('GROUPS PRUNED with limit', MAX_GROUPS)
            res = sorted([(len(g), g) for g in res])
            res = res[:MAX_GROUPS]
            res = set([x[1] for x in res])
        self.groups = res

    def get_groups(self):
        return [list(g) for g in self.groups]

def ma_split_groups(comm, groups, atoms, init, actions):
    private_groups = [x for x in groups if x[0].is_private]
    pub_groups = [x for x in groups if not x[0].is_private]
    #pub_groups = extend_groups(pub_groups, atoms, init, actions)

    print('Num pub groups:', len(pub_groups))
    if comm.is_master:
        print('Master')
        sys.stdout.flush()
        mg = MutexGroups(pub_groups)
        for i, grps in enumerate(comm.recvFromAll()):
            print('Master: update', i)
            sys.stdout.flush()
            grps = [[pddl.Atom(x[0], x[1]) for x in g] for g in grps]
            mg.update(grps)
        pub_groups = mg.get_groups()
        comm.sendToAll(public_groups(pub_groups))

    else:
        print('Slave')
        sys.stdout.flush()
        # Slave just sends its public groups to the master and waits for
        # the splitted groups
        comm.sendToMaster(public_groups(pub_groups))
        print('Sent to master')
        sys.stdout.flush()
        pub_groups = comm.recvFromMaster()
        pub_groups = [[pddl.Atom(x[0], x[1]) for x in g] for g in pub_groups]

    # Finally append private groups and return it
    groups = pub_groups + private_groups
    return groups

def compute_groups(task, atoms, actions,
                   reachable_action_params, partial_encoding=True,
                   comm = None):
    invariants, groups = invariant_finder.get_groups(task, reachable_action_params)

    with timers.timing("Instantiating groups"):
        groups = instantiate_groups(groups, task, atoms)

    if comm is not None:
        # Try to instantiate another mutex groups that are based on initial
        # states of other agents
        #groups = ma_instantiate_groups(comm, groups, invariants, task, atoms)

        # Separate private atoms to a separate groups
        groups = split_groups_by_private_atoms(groups)
        print('Groups split to private/public')
        sys.stdout.flush()

        # Split mutex groups so that all agents have the same mutex groups
        # -- this should ensure that no agent have invalid mutexes.
        groups = ma_split_groups(comm, groups, atoms, task.init, actions)
    else:
        print('NOT COMM!')
        sys.stdout.flush()
    print('Groups computed.')
    sys.stdout.flush()

    # Sort here already to get deterministic mutex groups.
    groups = sort_groups(groups)

    # TODO: I think that collect_all_mutex_groups should do the same thing
    #       as choose_groups with partial_encoding=False, so these two should
    #       be unified.
    with timers.timing("Collecting mutex groups"):
        mutex_groups = collect_all_mutex_groups(groups, atoms)
    with timers.timing("Choosing groups", block=True):
        groups = choose_groups(groups, atoms, partial_encoding=partial_encoding)
    groups = sort_groups(groups)

    with timers.timing("Building translation key"):
        translation_key = build_translation_key(groups)

    if DEBUG:
        for group in groups:
            if len(group) >= 2:
                print("{%s}" % ", ".join(map(str, group)))

    return groups, mutex_groups, translation_key
