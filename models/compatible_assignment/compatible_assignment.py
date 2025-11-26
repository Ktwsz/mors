import uuid
import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()
import mors_lib
mors_lib.model = model

class SolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, task):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.task = task

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        print('task = ' + (str([self.value(v) for v in self.task]) + '\n'), end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def analyse_all_different_43(x):
    model.Add(True)

def all_different_42(x):
    analyse_all_different_43(array1d(x))
    ortools_all_different(array1d(x))

def alldifferent_41(x):
    all_different_42(array1d(x))
n = 4
W = set(range(1, n + 1))
m = 3
T = set(range(1, 2 * m + 1))
profit = Array([W, T], Array([2, 3, 4, 2, 3, 1, 6, 6, 7, 3, 1, 2, 8, 7, 8, 4, 3, 1, 7, 7, 6, 3, 1, 2]))
compatible = Array([W, W], Array([True, False, False, False, False, True, False, False, False, False, True, True, False, False, True, True]))
task = IntVarArray('task', W, T)
alldifferent_41(task)
for w1 in W:
    for w2 in W:
        model.add_implication(and_(mors_lib_bool(task[w1] != m, task[w1] == m), mors_lib_bool(task[w2] == task[w1] + 1, task[w2] != task[w1] + 1)), compatible[w1, w2])
model.maximize(sum(Array([profit[w, task[w]] for w in W])))
solver = cp_model.CpSolver()
solution_printer = SolutionPrinter(task)
status = solver.solve(model, solution_printer)