import uuid
import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()
import mors_lib
mors_lib.model = model

class SolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, y):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.y = y

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        print('y=' + str([self.value(v) for v in self.y]), end='')
        print('\n', end='')

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
n = 5
PERSON = set(range(1, n + 1))
POS = set(range(1, n + 1))
friend = Array([PERSON, PERSON], Array([0, 4, 5, 8, -2, 4, 0, -1, 6, 0, 5, -1, 0, 9, -4, 8, 6, 9, 7, 6, -2, 0, -4, 6, 0]))
y = IntVarArray('y', POS, PERSON)
alldifferent_40(y)
model.maximize(sum(Array([friend[y[i], y[i + 1]] for i in range(1, n - 1 + 1)])))
solver = cp_model.CpSolver()
solution_printer = SolutionPrinter(y)
status = solver.solve(model, solution_printer)