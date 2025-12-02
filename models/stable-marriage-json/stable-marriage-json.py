import uuid
import math
from ortools.sat.python import cp_model
from mors_lib import *
import mors_lib
model = cp_model.CpModel()
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
n = load_from_json('n')
Men = set(range(1, max(range(1, n + 1)) + 1))
Women = set(range(1, max(range(1, n + 1)) + 1))
rankWomen = load_array_from_json('rankWomen', Women, Men)
rankMen = load_array_from_json('rankMen', Men, Women)
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