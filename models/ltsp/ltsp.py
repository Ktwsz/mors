import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()

class VarArraySolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, order, city):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.order = order
        self.city = city

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        print('order=' + (str([self.value(v) for v in self.order.values()]) + ('\ncity=' + (str([self.value(v) for v in self.city.values()]) + '\n'))), end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def fzn_inverse(f, invf):
    ortools_inverse(model, f, invf, min(index_set(f)), min(index_set(invf)))

def analyse_all_different_16(x):
    model.Add(True)

def inverse_43(f, invf):
    analyse_all_different_16(f)
    analyse_all_different_16(invf)
    fzn_inverse(f, invf)
n = 8
CITY = set(range(1, n + 1))
POS = set(range(1, n + 1))
coord = dict(zip(CITY, [-7, -4, -2, 0, 1, 6, 9, 12]))
m = 7
PREC = set(range(1, m + 1))
left = dict(zip(PREC, [3, 7, 8, 3, 2, 1, 4]))
right = dict(zip(PREC, [1, 2, 4, 6, 5, 7, 2]))
order = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(POS), 'order' + str(key)) for key in CITY}
city = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(CITY), 'city' + str(key)) for key in POS}
inverse_43(order, city)
for i in PREC:
    model.Add(order[left[i]] < order[right[i]])
model.minimize(sum([abs_(model, access(model, coord, city[i]) - access(model, coord, city[i + 1])) for i in range(1, n - 1 + 1)]))
solver = cp_model.CpSolver()
solution_printer = VarArraySolutionPrinter(order, city)
status = solver.solve(model, solution_printer)