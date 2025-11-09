import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()

class VarArraySolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, Products, produce, Resources, used):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.Products = Products
        self.produce = produce
        self.Resources = Resources
        self.used = used

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        for p in self.Products:
            print('' + (str(p) + (' = ' + (str(self.value(self.produce[p])) + ';\n'))), end='')
        for r in self.Resources:
            print('' + (str(r) + (' = ' + (str(self.value(self.used[r])) + ';\n'))), end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count
Products = set(range(1, 2 + 1))
profit = dict(zip(Products, [400, 450]))
Resources = set(range(1, 5 + 1))
capacity = dict(zip(Resources, [4000, 6, 2000, 500, 500]))
consumption = dict(zip(product(Products, Resources), [250, 2, 75, 100, 0, 200, 0, 150, 150, 75]))
mproducts = max([min([capacity[r] // consumption[p, r] for r in Resources if consumption[p, r] > 0]) for p in Products])
produce = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(range(0, mproducts + 1)), 'produce' + str(key)) for key in Products}
used = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(range(0, max(capacity.values()) + 1)), 'used' + str(key)) for key in Resources}
...
for r in Resources:
    model.Add(used[r] == sum([consumption[p, r] * produce[p] for p in Products]))
for r in Resources:
    model.Add(used[r] <= capacity[r])
model.maximize(sum([profit[p] * produce[p] for p in Products]))
solver = cp_model.CpSolver()
solution_printer = VarArraySolutionPrinter(Products, produce, Resources, used)
status = solver.solve(model, solution_printer)