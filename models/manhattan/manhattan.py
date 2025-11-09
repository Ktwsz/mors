import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()

class VarArraySolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, NUM, grid):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.NUM = NUM
        self.grid = grid

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        print('obj = ' + (format_37(obj) + ';\n'), end='')
        for i in self.NUM:
            for j in self.NUM:
                print((str(self.value(self.grid[i, j])) if grid[i, j] > 0 else '.') + ('\n' if j == n else ''), end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def global_cardinality_1(x, cover, lbound, ubound):
    return ...

def fzn_alldifferent_except(vs, S):

    def let_in_2():
        A = set()
        return global_cardinality_1(vs, set2array(A), [0 for i in A], [1 for i in A])
    if all([has_bounds(vs[i]) for i in vs.keys()]):
        let_in_2()
    else:
        for i in vs.keys():
            for j in vs.keys():
                if i < j:
                    model.Add(or_(model, and_(model, in_(model, vs[i], S), in_(model, vs[j], S)), mors_lib_bool(model, model.Add(vs[i] != vs[j]), model.Add(vs[i] == vs[j]))) == True)

def all_different_except_9(vs, S):
    fzn_alldifferent_except(array1d(vs), S)

def alldifferent_except_8(vs, S):
    all_different_except_9(vs, S)

def fzn_alldifferent_except_0(vs):
    alldifferent_except_8(vs, {0})

def all_different_except_0_39(vs):
    fzn_alldifferent_except_0(array1d(vs))

def format_37(x):
    str(self.value(x))

def alldifferent_except_0_38(vs):
    all_different_except_0_39(vs)
n = 5
NUM = set(range(1, n + 1))
x = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(NUM), 'x' + str(key)) for key in NUM}
y = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(NUM), 'y' + str(key)) for key in NUM}
dist = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(range(0, 2 * n - 2 + 1)), 'dist' + str(key)) for key in product(NUM, NUM)}
obj = model.new_int_var(-4611686018427387, 4611686018427387, 'obj')
grid = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(range(0, n + 1)), 'grid' + str(key)) for key in product(NUM, NUM)}
for i in NUM:
    for j in NUM:
        if i < j:
            model.Add(dist[i, j] >= max(i) - 1)
for i in NUM:
    model.Add(access(model, grid, (x[i], y[i])) == i)
alldifferent_except_0_38([grid[i, j] for i in NUM for j in NUM])
model.minimize(obj)
solver = cp_model.CpSolver()
solution_printer = VarArraySolutionPrinter(NUM, grid)
status = solver.solve(model, solution_printer)