import uuid
import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()
import mors_lib
mors_lib.model = model

class SolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, FACT, WARE, ship, W):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.FACT = FACT
        self.WARE = WARE
        self.ship = ship
        self.W = W

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        for f in self.FACT:
            for w in self.WARE:
                print(show_int(2, self.value(self.ship[f, w])) + ('\n' if w == self.W else ' '), end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count
W = 4
WARE = set(range(1, W + 1))
F = 3
FACT = set(range(1, F + 1))
demand = Array(WARE, Array([30, 20, 35, 20]))
production = Array(FACT, Array([40, 40, 25]))
cost = Array([FACT, WARE], Array([6, 5, 7, 9, 3, 2, 4, 1, 7, 3, 9, 5]))
ship = IntVarArray('ship', [FACT, WARE], None)
for f in FACT:
    for w in WARE:
        model.Add(ship[f, w] >= 0)
for w in WARE:
    model.Add(sum(Array([ship[f, w] for f in FACT])) >= demand[w])
for f in FACT:
    model.Add(sum(Array([ship[f, w] for w in WARE])) <= production[f])
model.minimize(sum(Array([cost[f, w] * ship[f, w] for f in FACT for w in WARE])))
solver = cp_model.CpSolver()
solution_printer = SolutionPrinter(FACT, WARE, ship, W)
status = solver.solve(model, solution_printer)