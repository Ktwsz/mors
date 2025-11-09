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
        print(str([self.value(v) for v in self.task.values()]), end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def analyse_all_different_42(x):
    model.Add(True)

def all_different_41(x):
    analyse_all_different_42(array1d(x))
    ortools_all_different(model, array1d(x))

def alldifferent_40(x):
    all_different_41(array1d(x))
n = 4
DOM = set(range(1, n + 1))
m = 5
COD = set(range(1, m + 1))
profit = dict(zip(product(DOM, COD), [7, 1, 3, 4, 6, 8, 2, 5, 1, 4, 4, 3, 7, 2, 5, 3, 1, 6, 3, 6]))
task = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(COD), 'task' + str(key)) for key in DOM}
alldifferent_40(task)
model.maximize(sum([access(model, profit, (w, task[w])) for w in DOM]))
solver = cp_model.CpSolver()
solution_printer = VarArraySolutionPrinter(task)
status = solver.solve(model, solution_printer)