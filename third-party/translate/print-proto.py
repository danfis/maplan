#!/usr/bin/env python

import sys
import os
if os.path.isdir('../protobuf/python/build/lib/google'):
    sys.path.insert(0, os.path.abspath('../protobuf/python/build/lib'))

import problemdef_pb2

def main(fn):
    prob = problemdef_pb2.PlanProblem()
    s = open(fn, 'rb').read()
    prob.ParseFromString(s)
    print(str(prob))

if __name__ == '__main__':
    main(sys.argv[1])
