import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()

class VarArraySolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, NUM, grid, n):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.NUM = NUM
        self.grid = grid
        self.n = n

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        print('obj = ' + (format_37(obj) + ';\n'), end='')
        for i in self.NUM:
            for j in self.NUM:
                print((str(self.value(self.grid[i, j])) if self.value(self.grid[i, j]) > 0 else '.') + ('\n' if j == self.n else ''), end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def count_eq_3(x, y, c):

    def let_in_5():
        def_ = assert_(has_bounds(y), 'Unable to decompose count_eq without bounds for the y argument', lb(y) - 1 if 0 in dom(y) else 0)
        return count_eq_3([default(i, def_) for i in x.values()], y, c)
    return let_in_5()

def count_eq_3(x, y, c):

    def let_in_5():
        def_ = assert_(has_bounds(y), 'Unable to decompose count_eq without bounds for the y argument', lb(y) - 1 if 0 in dom(y) else 0)
        return count_eq_3([default(i, def_) for i in x.values()], y, c)
    let_in_5()

def count_eq_2(x, y):

    def let_in_4():
        c = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(0, len(x) + 1)), 'c')
        count_eq_3(x, y, c)
        return c
    return let_in_4()

def count_32(x, y):
    return count_eq_2(x, y)

def fzn_count_leq(x, y, c):

    def let_in_3():
        z = count_32(x, y)
        return z >= c
    return let_in_3()

def fzn_count_geq(x, y, c):

    def let_in_7():
        z = count_32(x, y)
        return z <= c
    return let_in_7()

def fzn_count_leq_par(x, y, c):
    return fzn_count_leq(x, y, c)

def fzn_count_geq_par(x, y, c):
    return fzn_count_geq(x, y, c)

def count_leq_4(x, y, c):
    return fzn_count_leq_par(array1d(x), y, c)

def count_geq_5(x, y, c):
    return fzn_count_geq_par(array1d(x), y, c)

def count_0(x):

    def let_in_9():
        xx = array1d(x)
        return sum([bool2int(model, y) for y in xx.values()])
    return let_in_9()

def fzn_global_cardinality_low_up(x, cover, lbound, ubound):
    return all([count_leq_4([xi for xi in x.values()], cover[i], lbound[i]) if ubound[i] >= len(x) else count_geq_5([xi for xi in x.values()], cover[i], ubound[i]) if lbound[i] <= 0 else count_0([xi == cover[i] for xi in x.values()]) in range(lbound[i], ubound[i] + 1) for i in index_set(cover)])

def global_cardinality_1(x, cover, lbound, ubound):
    return assert_(index_sets_agree(cover, lbound) and index_sets_agree(cover, ubound), 'global_cardinality: ' + 'cover has index sets ' + show_index_sets(cover) + ', lbound has index sets ' + show_index_sets(lbound) + ', and ubound has index sets ' + show_index_sets(lbound) + ', but they must have identical index sets', assert_(all([l <= 0 for l in array1d(lbound)]) and all([u >= 0 for u in array1d(ubound)]) or len(cover) == 0, 'global_cardinality_low_up: ' + 'lbound and ubound must allow a count of 0 when x is empty, or also be empty', True) if len(x) == 0 else fzn_global_cardinality_low_up(array1d(x), array1d(cover), array1d(lbound), array1d(ubound)))

def fzn_alldifferent_except(vs, S):

    def let_in_2():
        A = set(dom_array(vs) - S)
        return global_cardinality_1(vs, set2array(A), [0 for i in A], [1 for i in A])
    if all([has_bounds(vs[i]) for i in index_set(vs)]):
        let_in_2()
    else:
        for i in index_set(vs):
            for j in index_set(vs):
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

def manhattan(x1, y1, x2, y2):
    return abs_(model, x1 - x2) + abs_(model, y1 - y2)
n = 5
NUM = set(range(1, n + 1))
x = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(NUM), 'x' + str(key)) for key in NUM}
y = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(NUM), 'y' + str(key)) for key in NUM}
dist = array2d(NUM, NUM, [manhattan(x[i], y[i], x[j], y[j]) if i < j else 0 for i in NUM for j in NUM])
obj = sum([dist[i, j] for i in NUM for j in NUM if i < j])
grid = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(range(0, n + 1)), 'grid' + str(key)) for key in product(NUM, NUM)}
for i in NUM:
    for j in NUM:
        if i < j:
            model.Add(dist[i, j] >= max(i, j) - 1)
for i in NUM:
    model.Add(access(model, grid, (x[i], y[i])) == i)
alldifferent_except_0_38([grid[i, j] for i in NUM for j in NUM])
model.minimize(obj)
solver = cp_model.CpSolver()
solution_printer = VarArraySolutionPrinter(NUM, grid, n)
status = solver.solve(model, solution_printer)