import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()

class VarArraySolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, x, POS, DIG, COPY):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.x = x
        self.POS = POS
        self.DIG = DIG
        self.COPY = COPY

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        print(str([self.value(v) for v in self.x]), end='')
        print('\n', end='')
        for p in self.POS:
            for d in self.DIG:
                for c in self.COPY:
                    print(str(d) + ' ' if x[d, c] == p else '', end='')

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
DIG = set(range(1, n + 1))
m = 3
COPY = set(range(1, m + 1))
l = m * n
POS = set(range(1, l + 1))
x = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(POS), 'x' + str(key)) for key in product(DIG, COPY)}
alldifferent_40([x[d, c] for d in DIG for c in COPY])
for d in DIG:
    for c in range(1, m - 1 + 1):
        model.Add(x[d, c + 1] == x[d, c] + d + 1)
solver = cp_model.CpSolver()
solution_printer = VarArraySolutionPrinter(x, POS, DIG, COPY)
status = solver.solve(model, solution_printer)