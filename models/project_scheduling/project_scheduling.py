import uuid
import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()
import mors_lib
mors_lib.model = model

class SolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, makespan, start):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.makespan = makespan
        self.start = start

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        print(str(self.value(self.makespan)), end='')
        print(' = ', end='')
        print(str([self.value(v) for v in self.start]), end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def max_1(x, y):

    def let_in_2():
        m = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(max(lb(x), lb(y)), max(ub(x), ub(y)) + 1)), 'm')
        int_max(x, y, m)
        return m
    return let_in_2()

def max_t(x):

    def let_in_3():
        m = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(lb_array(x), ub_array(x) + 1)), 'm')
        array_int_maximum(m, x)
        return m
    return 0 if len(x) == 0 else x[min(index_set(x))] if len(x) == 1 else max_1(x[1], x[2]) if len(x) == 2 else let_in_3()

def max_28(x):

    def let_in_1():
        xx = array1d(x)
        model.Add(len(x) >= 1)
        return max_t(xx)
    return let_in_1()
n = 8
TASK = set(range(1, n + 1))
foundations = 1
interior_walls = 2
exterior_walls = 3
chimney = 4
roof = 5
doors = 6
tiles = 7
windows = 8
duration = Array(TASK, Array([7, 4, 3, 3, 2, 2, 3, 3]))
p = 8
PREC = set(range(1, p + 1))
pre = Array([PREC, range(1, 2 + 1)], Array([foundations, interior_walls, foundations, exterior_walls, foundations, chimney, exterior_walls, roof, exterior_walls, windows, interior_walls, doors, chimney, tiles, roof, tiles]))
t = sum(duration)
start = IntVarArray('start', TASK, range(0, t + 1))
makespan = max_28(Array([start[t] + duration[t] for t in TASK]))
for i in PREC:
    model.Add(start[pre[i, 1]] + duration[pre[i, 1]] <= start[pre[i, 2]])
model.minimize(makespan)
solver = cp_model.CpSolver()
solution_printer = SolutionPrinter(makespan, start)
status = solver.solve(model, solution_printer)