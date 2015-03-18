from __future__ import print_function

SAS_FILE_VERSION = 3


class SASTask:
    def __init__(self, variables, mutexes, init, goal,
                 operators, axioms, metric, agents):
        self.variables = variables
        self.mutexes = mutexes
        self.init = init
        self.goal = goal
        self.operators = sorted(operators, key=lambda op: (op.name, op.prevail, op.pre_post))
        self.axioms = sorted(axioms, key=lambda axiom: (axiom.condition, axiom.effect))
        self.metric = metric
        self.agents = agents

    def output(self, stream):
        print("begin_version", file=stream)
        print(SAS_FILE_VERSION, file=stream)
        print("end_version", file=stream)
        print("begin_metric", file=stream)
        print(int(self.metric), file=stream)
        print("end_metric", file=stream)
        self.variables.output(stream)
        print(len(self.mutexes), file=stream)
        for mutex in self.mutexes:
            mutex.output(stream)
        self.init.output(stream)
        self.goal.output(stream)
        print(len(self.operators), file=stream)
        for op in self.operators:
            op.output(stream)
        print(len(self.axioms), file=stream)
        for axiom in self.axioms:
            axiom.output(stream)

        print("begin_agents", file=stream)
        print(len(self.agents), file=stream)
        for agent in self.agents:
            print(agent, file=stream)
        print("end_agents", file=stream)

    def output_proto(self, stream):
        import problemdef_pb2
        prob = problemdef_pb2.PlanProblem()
        prob.version = SAS_FILE_VERSION
        self.variables.output_proto(prob)
        for mutex in self.mutexes:
            mutex.output_proto(prob)
        self.init.output_proto(prob)
        self.goal.output_proto(prob)
        for op in self.operators:
            op.output_proto(self.metric, prob)

        for agent in self.agents:
            prob.agent_name.append(agent)

        s = prob.SerializeToString()
        stream.write(s)

    def get_encoding_size(self):
        task_size = 0
        task_size += self.variables.get_encoding_size()
        for mutex in self.mutexes:
            task_size += mutex.get_encoding_size()
        task_size += self.goal.get_encoding_size()
        for op in self.operators:
            task_size += op.get_encoding_size()
        for axiom in self.axioms:
            task_size += axiom.get_encoding_size()
        return task_size

class SASVariables:
    def __init__(self, ranges, axiom_layers, value_names, private_vars):
        self.ranges = ranges
        self.axiom_layers = axiom_layers
        self.value_names = value_names
        self.is_private = private_vars
    def dump(self):
        for var, (rang, axiom_layer) in enumerate(zip(self.ranges, self.axiom_layers)):
            if axiom_layer != -1:
                axiom_str = " [axiom layer %d]" % axiom_layer
            else:
                axiom_str = ""
            print("v%d in {%s}%s" % (var, list(range(rang)), axiom_str))
    def output(self, stream):
        print(len(self.ranges), file=stream)
        for var, (rang, axiom_layer, values) in enumerate(zip(
                self.ranges, self.axiom_layers, self.value_names)):
            print("begin_variable", file=stream)
            print("var%d" % var, file=stream)
            print(axiom_layer, file=stream)
            print(rang, file=stream)
            assert rang == len(values), (rang, values)
            for value in values:
                print(value, file=stream)
            print("end_variable", file=stream)

    def output_proto(self, prob):
        for var, (rang, axiom_layer, values, is_private) in enumerate(zip(
                self.ranges, self.axiom_layers, self.value_names, self.is_private)):
            protovar = prob.var.add()
            protovar.name = "var%d" % var
            protovar.range = rang
            for value in values:
                protovar.fact_name.append(value)
            if is_private is not None:
                protovar.is_private = is_private

    def get_encoding_size(self):
        # A variable with range k has encoding size k + 1 to also give the
        # variable itself some weight.
        return len(self.ranges) + sum(self.ranges)

class SASMutexGroup:
    def __init__(self, facts):
        self.facts = facts
    def dump(self):
        for var, val in self.facts:
            print("v%d: %d" % (var, val))
    def output(self, stream):
        print("begin_mutex_group", file=stream)
        print(len(self.facts), file=stream)
        for var, val in self.facts:
            print(var, val, file=stream)
        print("end_mutex_group", file=stream)

    def output_proto(self, prob):
        proto = prob.mutex.add()
        for var, val in self.facts:
            v = proto.fact.add()
            v.var = var
            v.val = val

    def get_encoding_size(self):
        return len(self.facts)

class SASInit:
    def __init__(self, values):
        self.values = values
    def dump(self):
        for var, val in enumerate(self.values):
            if val != -1:
                print("v%d: %d" % (var, val))
    def output(self, stream):
        print("begin_state", file=stream)
        for val in self.values:
            print(val, file=stream)
        print("end_state", file=stream)

    def output_proto(self, prob):
        proto = prob.init_state
        for val in self.values:
            proto.val.append(val)

class SASGoal:
    def __init__(self, pairs):
        self.pairs = sorted(pairs)
    def dump(self):
        for var, val in self.pairs:
            print("v%d: %d" % (var, val))
    def output(self, stream):
        print("begin_goal", file=stream)
        print(len(self.pairs), file=stream)
        for var, val in self.pairs:
            print(var, val, file=stream)
        print("end_goal", file=stream)

    def output_proto(self, prob):
        proto = prob.goal
        for var, val in self.pairs:
            v = proto.val.add()
            v.var = var
            v.val = val

    def get_encoding_size(self):
        return len(self.pairs)

class SASOperator:
    def __init__(self, name, prevail, pre_post, cost):
        self.name = name
        self.prevail = sorted(prevail)
        self.pre_post = sorted(pre_post)
        self.cost = cost
    def dump(self):
        print(self.name)
        print("Prevail:")
        for var, val in self.prevail:
            print("  v%d: %d" % (var, val))
        print("Pre/Post:")
        for var, pre, post, cond in self.pre_post:
            if cond:
                cond_str = " [%s]" % ", ".join(["%d: %d" % tuple(c) for c in cond])
            else:
                cond_str = ""
            print("  v%d: %d -> %d%s" % (var, pre, post, cond_str))
    def output(self, stream):
        print("begin_operator", file=stream)
        print(self.name[1:-1], file=stream)
        print(len(self.prevail), file=stream)
        for var, val in self.prevail:
            print(var, val, file=stream)
        print(len(self.pre_post), file=stream)
        for var, pre, post, cond in self.pre_post:
            print(len(cond), end=' ', file=stream)
            for cvar, cval in cond:
                print(cvar, cval, end=' ', file=stream)
            print(var, pre, post, file=stream)
        print(self.cost, file=stream)
        print("end_operator", file=stream)

    def output_proto(self, use_metric, prob):
        proto = prob.operator.add()
        proto.name = self.name[1:-1]
        if use_metric:
            proto.cost = self.cost
        else:
            proto.cost = 1

        ppre = proto.pre
        peff = proto.eff
        for var, val in self.prevail:
            v = ppre.val.add()
            v.var = var
            v.val = val

        for var, pre, post, cond in self.pre_post:
            if pre >= 0:
                v = ppre.val.add()
                v.var = var
                v.val = pre

            if len(cond) > 0:
                pcond = proto.cond_eff.add()
                for cvar, cval in cond:
                    v = pcond.pre.val.add()
                    v.var = cvar
                    v.val = cval
                v = pcond.eff.val.add()
                v.var = var
                v.val = post

            else:
                if post >= 0:
                    v = peff.val.add()
                    v.var = var
                    v.val = post

    def get_encoding_size(self):
        size = 1 + len(self.prevail)
        for var, pre, post, cond in self.pre_post:
            size += 1 + len(cond)
            if pre != -1:
                size += 1
        return size

class SASAxiom:
    def __init__(self, condition, effect):
        self.condition = sorted(condition)
        self.effect = effect
        assert self.effect[1] in (0, 1)

        for _, val in condition:
            assert val >= 0, condition
    def dump(self):
        print("Condition:")
        for var, val in self.condition:
            print("  v%d: %d" % (var, val))
        print("Effect:")
        var, val = self.effect
        print("  v%d: %d" % (var, val))
    def output(self, stream):
        print("begin_rule", file=stream)
        print(len(self.condition), file=stream)
        for var, val in self.condition:
            print(var, val, file=stream)
        var, val = self.effect
        print(var, 1 - val, val, file=stream)
        print("end_rule", file=stream)
    def get_encoding_size(self):
        return 1 + len(self.condition)
