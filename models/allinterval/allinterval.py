import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()

class VarArraySolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, x):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.x = x

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        print('x = ' + (str([self.value(v) for v in self.x]) + ';\n'), end='')

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
n = 10
x = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(range(1, n + 1)), 'x' + str(key)) for key in range(1, n + 1)}
u = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(range(1, n - 1 + 1)), 'u' + str(key)) for key in range(1, n - 1 + 1)}
alldifferent_41(x)
alldifferent_41(u)
for i in range(1, n - 1 + 1):
    model.Add(u[i] == abs_(model, x[i + 1] - x[i]))
solver = cp_model.CpSolver()
solution_printer = VarArraySolutionPrinter(x)
status = solver.solve(model, solution_printer)