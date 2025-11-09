import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()

class VarArraySolutionPrinter(cp_model.CpSolverSolutionCallback):

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
rankWomen = dict(zip(product(Women, Men), [1, 2, 4, 3, 5, 3, 5, 1, 2, 4, 5, 4, 2, 1, 3, 1, 3, 5, 4, 2, 4, 2, 3, 5, 1]))
rankMen = dict(zip(product(Men, Women), [5, 1, 2, 4, 3, 4, 1, 3, 2, 5, 5, 3, 2, 4, 1, 1, 5, 4, 3, 2, 4, 3, 2, 1, 5]))
wife = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(Women), 'wife' + str(key)) for key in Men}
husband = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(Men), 'husband' + str(key)) for key in Women}
for m in Men:
    model.Add(access(model, husband, wife[m]) == m)
for w in Women:
    model.Add(access(model, wife, husband[w]) == w)
for m in Men:
    for o in Women:
        model.Add(impl_(model, mors_lib_bool(model, model.Add(rankMen[m, o] < access(model, rankMen, (m, wife[m]))), model.Add(rankMen[m, o] >= access(model, rankMen, (m, wife[m])))), mors_lib_bool(model, model.Add(access(model, rankWomen, (o, husband[o])) < rankWomen[o, m]), model.Add(access(model, rankWomen, (o, husband[o])) >= rankWomen[o, m]))) == True)
for w in Women:
    for o in Men:
        model.Add(impl_(model, mors_lib_bool(model, model.Add(rankWomen[w, o] < access(model, rankWomen, (w, husband[w]))), model.Add(rankWomen[w, o] >= access(model, rankWomen, (w, husband[w])))), mors_lib_bool(model, model.Add(access(model, rankMen, (o, wife[o])) < rankMen[o, w]), model.Add(access(model, rankMen, (o, wife[o])) >= rankMen[o, w]))) == True)
solver = cp_model.CpSolver()
solution_printer = VarArraySolutionPrinter(wife, husband)
status = solver.solve(model, solution_printer)