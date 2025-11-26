import uuid
import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()
import mors_lib
mors_lib.model = model

class SolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, wife, husband):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.wife = wife
        self.husband = husband

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        print('wives= ' + (str([self.value(v) for v in self.wife]) + ('\nhusbands= ' + (str([self.value(v) for v in self.husband]) + '\n'))), end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count
n = 5
Men = set(range(1, max(range(1, n + 1)) + 1))
Women = set(range(1, max(range(1, n + 1)) + 1))
rankWomen = Array([Women, Men], Array([1, 2, 4, 3, 5, 3, 5, 1, 2, 4, 5, 4, 2, 1, 3, 1, 3, 5, 4, 2, 4, 2, 3, 5, 1]))
rankMen = Array([Men, Women], Array([5, 1, 2, 4, 3, 4, 1, 3, 2, 5, 5, 3, 2, 4, 1, 1, 5, 4, 3, 2, 4, 3, 2, 1, 5]))
wife = IntVarArray('wife', Men, Women)
husband = IntVarArray('husband', Women, Men)
for m in Men:
    model.Add(husband[wife[m]] == m)
for w in Women:
    model.Add(wife[husband[w]] == w)
for m in Men:
    for o in Women:
        model.add_implication(mors_lib_bool(rankMen[m, o] < rankMen[m, wife[m]], rankMen[m, o] >= rankMen[m, wife[m]]), mors_lib_bool(rankWomen[o, husband[o]] < rankWomen[o, m], rankWomen[o, husband[o]] >= rankWomen[o, m]))
for w in Women:
    for o in Men:
        model.add_implication(mors_lib_bool(rankWomen[w, o] < rankWomen[w, husband[w]], rankWomen[w, o] >= rankWomen[w, husband[w]]), mors_lib_bool(rankMen[o, wife[o]] < rankMen[o, w], rankMen[o, wife[o]] >= rankMen[o, w]))
solver = cp_model.CpSolver()
solution_printer = SolutionPrinter(wife, husband)
status = solver.solve(model, solution_printer)