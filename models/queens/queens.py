import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()

class VarArraySolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, n):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.n = n

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        for i in range(1, self.n + 1):
            for j in range(1, self.n + 1):
                print(('Q ' if q[i] == j else '. ') + ('\n' if j == n else ''), end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def analyse_all_different_42(x):
    model.Add(True)

def all_different_41(x):
    analyse_all_different_42(array1d(x))
    all_different(model, array1d(x))

def alldifferent_40(x):
    all_different_41(array1d(x))
n = 10
q = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(range(1, n + 1)), 'q' + str(key)) for key in range(1, n + 1)}
alldifferent_40(q)
alldifferent_40([q[i] + i for i in range(1, n + 1)])
alldifferent_40([q[i] - i for i in range(1, n + 1)])
solver = cp_model.CpSolver()
solution_printer = VarArraySolutionPrinter(n)
status = solver.solve(model, solution_printer)