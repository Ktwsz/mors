import uuid
import math
from ortools.sat.python import cp_model
from mors_lib import *
import mors_lib
model = cp_model.CpModel()
mors_lib.model = model

class SolutionPrinter(cp_model.CpSolverSolutionCallback):

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
profit = Array(OBJ, Array([1300, 1000, 520, 480, 325]))
size = Array(OBJ, Array([90, 72, 43, 40, 33]))
x = IntVarArray('x', OBJ, None)
for i in OBJ:
    model.Add(x[i] >= 0)
model.Add(sum(Array([size[i] * x[i] for i in OBJ])) <= capacity)
model.maximize(sum(Array([profit[i] * x[i] for i in OBJ])))
solver = cp_model.CpSolver()
solution_printer = SolutionPrinter(x)
status = solver.solve(model, solution_printer)