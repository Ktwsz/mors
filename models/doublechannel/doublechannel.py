import uuid
import math
from ortools.sat.python import cp_model
from mors_lib import *
import mors_lib
model = cp_model.CpModel()
mors_lib.model = model

class SolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, start, end, channel, next):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.start = start
        self.end = end
        self.channel = channel
        self.next = next

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        print('start = ', end='')
        print(str([self.value(v) for v in self.start]), end='')
        print(';\nend = ', end='')
        print(str([self.value(v) for v in self.end]), end='')
        print(';\nchannel = ', end='')
        print(str([self.value(v) for v in self.channel]), end='')
        print(';\nnext = ', end='')
        print(str([self.value(v) for v in self.next]), end='')
        print(';\n', end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def analyse_all_different_42(x):
    model.Add(True)

def fzn_if_then_else_var_bool(c, x, y):

    def let_in_1():
        d = BoolVarArray('d', index_set(c))
        for i in index_set(c):
            if i > min(index_set(c)):
                model.Add(d[i] == and_(~c[i - 1], d[i - 1]))
            else:
                model.Add(d[i] == mors_lib_bool(True, False))
        for i in index_set(c):
            model.add_implication(and_(c[i], d[i]), model.Add(y == x[i]))
    let_in_1()

def all_different_41(x):
    analyse_all_different_42(array1d(x))
    ortools_all_different(array1d(x))

def if_then_else(c, x, y):
    fzn_if_then_else_var_bool(c, x, y)

def alldifferent_40(x):
    all_different_41(array1d(x))
TYPE = set(range(1, 3 + 1))
entering = 1
leaving = 2
dummy = 3
nC = 2
CHANNEL = set(range(1, nC + 1))
len = Array(CHANNEL, Array([6, 8]))
nS = 8
SHIP = set(range(1, nS + 1))
SHIPE = set(range(1, nS + nC + 1))
speed = Array(SHIP, Array([1, 2, 1, 2, 1, 3, 1, 2]))
desired = Array(SHIP, Array([4, 4, 4, 4, 7, 7, 7, 7]))
dirn = Array(SHIP, Array([1, 2, 2, 1, 2, 2, 1, 1]))
leeway = 2
maxt = 100
TIME = set(range(0, maxt + 1))
kind = dirn + Array([dummy for i in range(1, nC + 1)])
start = IntVarArray('start', SHIPE, TIME)
end = IntVarArray('end', SHIPE, TIME)
channel = IntVarArray('channel', SHIPE, CHANNEL)
next = IntVarArray('next', SHIP, SHIPE)
for s in range(nS + 1, nS + nC + 1):
    model.Add(start[s] == maxt)
    model.Add(end[s] == maxt)
for s in range(nS + 1, nS + nC + 1):
    model.Add(channel[s] == s - nS)
for s in SHIP:
    model.Add(end[s] == start[s] + len[channel[s]] * speed[s])
alldifferent_40(next)
for s in SHIP:
    if_then_else(Array([mors_lib_bool(kind[s] + kind[next[s]] == entering + leaving, kind[s] + kind[next[s]] != entering + leaving), mors_lib_bool(True, False)]), Array([mors_lib_bool(end[s] <= start[next[s]], end[s] > start[next[s]]), and_(mors_lib_bool(start[s] + speed[s] * leeway <= start[next[s]], start[s] + speed[s] * leeway > start[next[s]]), mors_lib_bool(end[s] + speed[s] * leeway <= end[next[s]], end[s] + speed[s] * leeway > end[next[s]]))]), ite_result_2137)
for s in SHIP:
    model.Add(channel[next[s]] == channel[s])
model.minimize(sum(Array([abs_(start[s] - desired[s]) for s in SHIP])))
solver = cp_model.CpSolver()
solution_printer = SolutionPrinter(start, end, channel, next)
status = solver.solve(model, solution_printer)