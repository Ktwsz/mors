import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()

class VarArraySolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, totalcost, PRISONER, r, c):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.totalcost = totalcost
        self.PRISONER = PRISONER
        self.r = r
        self.c = c

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        print('cost = ' + (str(self.value(self.totalcost)) + '\n'), end='')
        for p in self.PRISONER:
            print('Prisoner ' + (str(p) + ' ') + ('(F)' if p in female else '(M)') + (' in [' + (str(self.value(self.r[p])) + (',' + (str(self.value(self.c[p])) + '].\n')))), end='')

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
k = 10
PRISONER = set(range(1, k + 1))
n = 4
ROW = set(range(1, n + 1))
m = 4
COL = set(range(1, m + 1))
danger = set({1, 3, 8})
female = set({1, 2, 3, 4, 5})
male = set()
cost = dict(zip(product(ROW, COL), [3, 4, 5, 8, 1, 3, 5, 6, 4, 5, 3, 2, 7, 4, 5, 6]))
r = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(ROW), 'r' + str(key)) for key in PRISONER}
c = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(COL), 'c' + str(key)) for key in PRISONER}
totalcost = model.new_int_var(-4611686018427387, 4611686018427387, 'totalcost')
alldifferent_41([r[p] * m + c[p] for p in PRISONER])
for p in PRISONER:
    for d in danger:
        if p != d:
            model.Add(abs_(model, r[p] - r[d]) + abs_(model, c[p] - c[d]) > 1)
for p in female:
    model.Add(r[p] <= (n + 1) // 2)
for p in male:
    model.Add(r[p] >= n // 2 + 1)
model.minimize(totalcost)
solver = cp_model.CpSolver()
solution_printer = VarArraySolutionPrinter(totalcost, PRISONER, r, c)
status = solver.solve(model, solution_printer)