import uuid
import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()
import mors_lib
mors_lib.model = model

class SolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, obj, NUM, grid, n):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.obj = obj
        self.NUM = NUM
        self.grid = grid
        self.n = n

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        print('obj = ' + (str(self.value(self.obj)) + ';\n'), end='')
        for i in self.NUM:
            for j in self.NUM:
                print((str(self.value(self.grid[i, j])) if self.value(self.grid[i, j]) > 0 else '.') + ('\n' if j == self.n else ''), end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def fzn_count_eq(x, y, c):
    ortools_count_eq(x, y, c)

def count_eq_3(x, y, c):
    fzn_count_eq(array1d(x), y, c)

def count_eq_2(x, y):

    def let_in_4():
        c = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(0, len(x) + 1)), 'c')
        count_eq_3(x, y, c)
        return c
    return let_in_4()

def fzn_global_cardinality_low_up_reif(x, cover, lbound, ubound):
    b = model.new_bool_var(str(uuid.uuid4()))
    model.Add(b == forall_(Array([in_(count_eq_2(x, cover[i]), range(lbound[i], ubound[i] + 1)) for i in index_set(cover)])))
    return b

def global_cardinality_1(x, cover, lbound, ubound):
    model.Add(assert_(index_sets_agree(cover, lbound) and index_sets_agree(cover, ubound), 'global_cardinality: ' + 'cover has index sets ' + show_index_sets(cover) + ', lbound has index sets ' + show_index_sets(lbound) + ', and ubound has index sets ' + show_index_sets(lbound) + ', but they must have identical index sets', assert_(all(Array([l <= 0 for l in array1d(lbound)])) and all(Array([u >= 0 for u in array1d(ubound)])) or len(cover) == 0, 'global_cardinality_low_up: ' + 'lbound and ubound must allow a count of 0 when x is empty, or also be empty', mors_lib_bool(True, False)) if len(x) == 0 else fzn_global_cardinality_low_up_reif(array1d(x), array1d(cover), array1d(lbound), array1d(ubound))) == True)

def fzn_alldifferent_except(vs, S):

    def let_in_2():
        A = set(dom_array(vs) - S)
        global_cardinality_1(vs, Array(A), Array([0 for i in A]), Array([1 for i in A]))
    if all(Array([has_bounds(vs[i]) for i in index_set(vs)])):
        let_in_2()
    else:
        for i in index_set(vs):
            for j in index_set(vs):
                if i < j:
                    model.add_bool_or(and_(in_(vs[i], S), in_(vs[j], S)), mors_lib_bool(vs[i] != vs[j], vs[i] == vs[j]))

def all_different_except_9(vs, S):
    fzn_alldifferent_except(array1d(vs), S)

def alldifferent_except_8(vs, S):
    all_different_except_9(vs, S)

def fzn_alldifferent_except_0(vs):
    alldifferent_except_8(vs, {0})

def all_different_except_0_39(vs):
    fzn_alldifferent_except_0(array1d(vs))

def alldifferent_except_0_38(vs):
    all_different_except_0_39(vs)

def manhattan(x1, y1, x2, y2):
    return abs_(x1 - x2) + abs_(y1 - y2)
n = 5
NUM = set(range(1, n + 1))
x = IntVarArray('x', NUM, NUM)
y = IntVarArray('y', NUM, NUM)
dist = array2d(NUM, NUM, Array([manhattan(x[i], y[i], x[j], y[j]) if i < j else 0 for i in NUM for j in NUM]))
obj = sum(Array([dist[i, j] for i in NUM for j in NUM if i < j]))
grid = IntVarArray('grid', [NUM, NUM], range(0, n + 1))
for i in NUM:
    for j in NUM:
        if i < j:
            model.Add(dist[i, j] >= max(i, j) - 1)
for i in NUM:
    model.Add(grid[x[i], y[i]] == i)
alldifferent_except_0_38(Array([grid[i, j] for i in NUM for j in NUM]))
model.minimize(obj)
solver = cp_model.CpSolver()
solution_printer = SolutionPrinter(obj, NUM, grid, n)
status = solver.solve(model, solution_printer)