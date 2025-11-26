import uuid
import math
from ortools.sat.python import cp_model
from mors_lib import *
import mors_lib
model = cp_model.CpModel()
mors_lib.model = model

class SolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, worker):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.worker = worker

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        print(str([self.value(v) for v in self.worker]), end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def analyse_all_different_42(x):
    model.Add(True)

def all_different_41(x):
    analyse_all_different_42(array1d(x))
    ortools_all_different(array1d(x))

def alldifferent_40(x):
    all_different_41(array1d(x))
n = 4
DOM = set(range(1, n + 1))
m = 4
COD = set(range(1, m + 1))
profit = Array([DOM, COD], Array([7, 1, 3, 4, 8, 2, 5, 1, 4, 3, 7, 2, 3, 1, 6, 3]))
worker = IntVarArray('worker', COD, DOM)
alldifferent_40(worker)
model.maximize(sum(Array([profit[worker[t], t] for t in COD])))
solver = cp_model.CpSolver()
solution_printer = SolutionPrinter(worker)
status = solver.solve(model, solution_printer)