import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()

class VarArraySolutionPrinter(cp_model.CpSolverSolutionCallback):

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
    all_different(model, array1d(x))

def alldifferent_41(x):
    all_different_42(array1d(x))
n = 4
W = set(range(1, n + 1))
m = 3
T = set(range(1, 2 * m + 1))
profit = dict(zip(product(W, T), [2, 3, 4, 2, 3, 1, 6, 6, 7, 3, 1, 2, 8, 7, 8, 4, 3, 1, 7, 7, 6, 3, 1, 2]))
compatible = dict(zip(product(W, W), [True, False, False, False, False, True, False, False, False, False, True, True, False, False, True, True]))
task = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(T), 'task' + str(key)) for key in W}
alldifferent_41(task)
for w1 in W:
    for w2 in W:
        model.Add(impl_(model, and_(model, mors_lib_bool(model, model.Add(task[w1] != m), model.Add(task[w1] == m)), mors_lib_bool(model, model.Add(task[w2] == task[w1] + 1), model.Add(task[w2] != task[w1] + 1))), compatible[w1, w2]) == True)
model.maximize(sum([profit[w, task[w]] for w in W]))
solver = cp_model.CpSolver()
solution_printer = VarArraySolutionPrinter(task)
status = solver.solve(model, solution_printer)