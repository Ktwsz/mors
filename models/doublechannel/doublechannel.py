import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()

class VarArraySolutionPrinter(cp_model.CpSolverSolutionCallback):

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
        print(str([self.value(v) for v in self.start.values()]), end='')
        print(';\nend = ', end='')
        print(str([self.value(v) for v in self.end.values()]), end='')
        print(';\nchannel = ', end='')
        print(str([self.value(v) for v in self.channel.values()]), end='')
        print(';\nnext = ', end='')
        print(str([self.value(v) for v in self.next.values()]), end='')
        print(';\n', end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def analyse_all_different_42(x):
    model.Add(True)

def fzn_if_then_else_var_bool(c, x, y):

    def let_in_1():
        d = {key: model.new_int_var(-4611686018427387, 4611686018427387, 'd' + str(key)) for key in index_set(c)}
        return and_(model, forall_(model, [model.Add(d[i] == and_(model, ~c[i - 1], d[i - 1])) if mors_lib_bool(model, model.Add(i > min(index_set(c))), model.Add(i <= min(index_set(c)))) else model.Add(d[i] == mors_lib_bool(model, model.Add(True), model.Add(False))) for i in index_set(c)]), forall_(model, [impl_(model, and_(model, c[i], d[i]), model.Add(y == x[i])) for i in index_set(c)]))
    let_in_1()

def all_different_41(x):
    analyse_all_different_42(array1d(x))
    ortools_all_different(model, array1d(x))

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
len = dict(zip(CHANNEL, [6, 8]))
nS = 8
SHIP = set(range(1, nS + 1))
SHIPE = set(range(1, nS + nC + 1))
speed = dict(zip(SHIP, [1, 2, 1, 2, 1, 3, 1, 2]))
desired = dict(zip(SHIP, [4, 7, 7, 7, 7]))
dirn = dict(zip(SHIP, [1, 2, 2, 1, 2, 2, 1, 1]))
leeway = 2
maxt = 100
TIME = set(range(0, maxt + 1))
kind = dirn + [dummy for i in range(1, nC + 1)]
start = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(TIME), 'start' + str(key)) for key in SHIPE}
end = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(TIME), 'end' + str(key)) for key in SHIPE}
channel = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(CHANNEL), 'channel' + str(key)) for key in SHIPE}
next = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(SHIPE), 'next' + str(key)) for key in SHIP}
for s in range(nS + 1, nS + nC + 1):
    model.Add(start[s] == maxt)
    model.Add(end[s] == maxt)
for s in range(nS + 1, nS + nC + 1):
    model.Add(channel[s] == s - nS)
for s in SHIP:
    model.Add(end[s] == start[s] + access(model, len, channel[s]) * speed[s])
alldifferent_40(next)
for s in SHIP:
    if_then_else([mors_lib_bool(model, model.Add(kind[s] + access(model, kind, next[s]) == entering + leaving), model.Add(kind[s] + access(model, kind, next[s]) != entering + leaving)), mors_lib_bool(model, model.Add(True), model.Add(False))], [mors_lib_bool(model, model.Add(end[s] <= access(model, start, next[s])), model.Add(end[s] > access(model, start, next[s]))), and_(model, mors_lib_bool(model, model.Add(start[s] + speed[s] * leeway <= access(model, start, next[s])), model.Add(start[s] + speed[s] * leeway > access(model, start, next[s]))), mors_lib_bool(model, model.Add(end[s] + speed[s] * leeway <= access(model, end, next[s])), model.Add(end[s] + speed[s] * leeway > access(model, end, next[s]))))], ite_result_2137)
for s in SHIP:
    model.Add(access(model, channel, next[s]) == channel[s])
model.minimize(sum([abs_(model, start[s] - desired[s]) for s in SHIP]))
solver = cp_model.CpSolver()
solution_printer = VarArraySolutionPrinter(start, end, channel, next)
status = solver.solve(model, solution_printer)