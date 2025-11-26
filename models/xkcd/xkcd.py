import uuid
import math
from ortools.sat.python import cp_model
from mors_lib import *
import mors_lib
model = cp_model.CpModel()
mors_lib.model = model

class SolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, menu_length, menu_names, order):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.menu_length = menu_length
        self.menu_names = menu_names
        self.order = order

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        for i in range(1, self.menu_length + 1):
            print(self.menu_names[i] + ': ' + str(self.value(self.order[i])) + '\n', end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count
menu_length = 6
money_limit = 1505
menu_prices = Array(range(1, menu_length + 1), Array([215, 275, 335, 355, 420, 580]))
menu_names = Array(range(1, menu_length + 1), Array(['fruit', 'fries', 'salad', 'wings', 'sticks', 'sampler']))
order = IntVarArray('order', range(1, menu_length + 1), None)
for i in range(1, menu_length + 1):
    model.Add(order[i] >= 0)
model.Add(sum(Array([order[i] * menu_prices[i] for i in range(1, menu_length + 1)])) == money_limit)
solver = cp_model.CpSolver()
solution_printer = SolutionPrinter(menu_length, menu_names, order)
status = solver.solve(model, solution_printer)