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
        print('x = ' + (str([self.value(v) for v in self.x]) + ';\n'), end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def analyse_all_different_43(x):
    model.Add(True)

def all_different_42(x):
    analyse_all_different_43(array1d(x))
    ortools_all_different(model, array1d(x))

def min_14(x, y):

    def let_in_2():
        m = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(min(lb(x), lb(y)), min(ub(x), ub(y)) + 1)), 'm')
        int_min(model, x, y, m)
        return m
    return let_in_2()

def alldifferent_41(x):
    all_different_42(array1d(x))
n = 10
y = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(range(1, n + 1)), 'y' + str(key)) for key in range(1, n + 1)}
v = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(range(1, n - 1 + 1)), 'v' + str(key)) for key in range(1, n - 1 + 1)}
x = [sum([i * bool2int(model, mors_lib_bool(model, model.Add(y[j] == i), model.Add(y[j] != i))) for i in range(1, n + 1)]) for j in range(1, n + 1)]
alldifferent_41(y)
alldifferent_41(v)
for i in range(1, n + 1):
    for j in range(1, n + 1):
        if i < j:
            model.Add(impl_(model, mors_lib_bool(model, model.Add(y[i] - y[j] == 1), model.Add(y[i] - y[j] != 1)), mors_lib_bool(model, model.Add(v[j - i] == y[j]), model.Add(v[j - i] != y[j]))) == True)
            model.Add(impl_(model, mors_lib_bool(model, model.Add(y[j] - y[i] == 1), model.Add(y[j] - y[i] != 1)), mors_lib_bool(model, model.Add(v[j - i] == y[i]), model.Add(v[j - i] != y[i]))) == True)
model.Add(abs_(model, y[1] - y[n]) == 1)
model.Add(v[n - 1] == min_14(y[1], y[n]))
solver = cp_model.CpSolver()
solution_printer = VarArraySolutionPrinter(x)
status = solver.solve(model, solution_printer)