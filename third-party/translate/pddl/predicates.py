from . import pddl_types

class Predicate(object):
    def __init__(self, name, arguments, private = False):
        self.name = name
        self.arguments = arguments
        self.is_private = private
    def parse(alist, private = False):
        name = alist[0]
        arguments = pddl_types.parse_typed_list(alist[1:], only_variables=True)
        return Predicate(name, arguments, private)
    parse = staticmethod(parse)
    def __str__(self):
        return "%s%s(%s)" % (('', '(P)')[int(self.is_private)],
                             self.name, ", ".join(map(str, self.arguments)))
