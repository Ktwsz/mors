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
        print('x = ', end='')
        print(str([self.value(v) for v in self.x]), end='')
        print('\n', end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count
n = 5
OBJ = set(range(1, n + 1))
capacity = 200
profit = dict(zip(OBJ, [1300, 1000, 520, 480, 325]))
size = dict(zip(OBJ, [90, 72, 43, 40, 33]))
x = {key: model.new_int_var(-4611686018427387, 4611686018427387, 'x' + str(key)) for key in OBJ}
for i in OBJ:
    model.Add(x[i] >= 0)
model.Add(sum([size[i] * x[i] for i in OBJ]) <= capacity)
model.maximize(sum([profit[i] * x[i] for i in OBJ]))
solver = cp_model.CpSolver()
solution_printer = VarArraySolutionPrinter(x)
status = solver.solve(model, solution_printer)