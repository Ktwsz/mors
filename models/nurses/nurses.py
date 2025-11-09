import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()

class VarArraySolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, NURSE, DAY, code, x):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.NURSE = NURSE
        self.DAY = DAY
        self.code = code
        self.x = x

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        for n in self.NURSE:
            for d in self.DAY:
                print(self.value(self.code[self.value(self.x[n, d])]) + ('\n' if d == m else ' '), end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def global_cardinality_1(x, cover, lbound, ubound):
    ...

def global_cardinality_low_up_35(x, cover, lbound, ubound):
    global_cardinality_1(x, cover, lbound, ubound)
k = 6
NURSE = set(range(1, k + 1))
m = 7
DAY = set(range(1, m + 1))
SHIFT = set(range(1, 3 + 1))
day = 1
night = 2
dayoff = 3
code = dict(zip(SHIFT, ['d', 'n', '-']))
o = 3
l = 2
u = 3
x = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(SHIFT), 'x' + str(key)) for key in product(NURSE, DAY)}
for n in NURSE:
    for d in range(1, m - 2 + 1):
        model.Add(impl_(model, and_(model, mors_lib_bool(model, model.Add(x[n, d] == night), model.Add(x[n, d] != night)), mors_lib_bool(model, model.Add(x[n, d + 1] == night), model.Add(x[n, d + 1] != night))), mors_lib_bool(model, model.Add(x[n, d + 2] == dayoff), model.Add(x[n, d + 2] != dayoff))) == True)
for n in NURSE:
    for d in range(1, m - 1 + 1):
        model.Add(impl_(model, mors_lib_bool(model, model.Add(x[n, d] == night), model.Add(x[n, d] != night)), mors_lib_bool(model, model.Add(x[n, d + 1] != day), model.Add(x[n, d + 1] == day))) == True)
for d in DAY:
    global_cardinality_low_up_35([x[n, d] for n in NURSE], [day, night], [o, l], [o, u])
solver = cp_model.CpSolver()
solution_printer = VarArraySolutionPrinter(NURSE, DAY, code, x)
status = solver.solve(model, solution_printer)