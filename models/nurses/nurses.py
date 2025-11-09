import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()

class VarArraySolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, NURSE, DAY, code, x, m):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.NURSE = NURSE
        self.DAY = DAY
        self.code = code
        self.x = x
        self.m = m

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        for n in self.NURSE:
            for d in self.DAY:
                print(self.value(self.code[self.value(self.x[n, d])]) + ('\n' if d == self.m else ' '), end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def count_eq_3(x, y, c):

    def let_in_3():
        def_ = assert_(has_bounds(y), 'Unable to decompose count_eq without bounds for the y argument', lb(y) - 1 if 0 in dom(y) else 0)
        return count_eq_3([default(i, def_) for i in x.values()], y, c)
    return let_in_3()

def count_eq_3(x, y, c):

    def let_in_3():
        def_ = assert_(has_bounds(y), 'Unable to decompose count_eq without bounds for the y argument', lb(y) - 1 if 0 in dom(y) else 0)
        return count_eq_3([default(i, def_) for i in x.values()], y, c)
    let_in_3()

def count_eq_2(x, y):

    def let_in_2():
        c = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(0, len(x) + 1)), 'c')
        count_eq_3(x, y, c)
        return c
    return let_in_2()

def count_30(x, y):
    return count_eq_2(x, y)

def fzn_count_leq(x, y, c):

    def let_in_1():
        z = count_30(x, y)
        return z >= c
    return let_in_1()

def fzn_count_geq(x, y, c):

    def let_in_5():
        z = count_30(x, y)
        return z <= c
    return let_in_5()

def fzn_count_leq_par(x, y, c):
    return fzn_count_leq(x, y, c)

def fzn_count_geq_par(x, y, c):
    return fzn_count_geq(x, y, c)

def count_leq_4(x, y, c):
    return fzn_count_leq_par(array1d(x), y, c)

def count_geq_5(x, y, c):
    return fzn_count_geq_par(array1d(x), y, c)

def count_0(x):

    def let_in_7():
        xx = array1d(x)
        return sum([bool2int(model, y) for y in xx.values()])
    return let_in_7()

def fzn_global_cardinality_low_up(x, cover, lbound, ubound):
    return all([count_leq_4([xi for xi in x.values()], cover[i], lbound[i]) if ubound[i] >= len(x) else count_geq_5([xi for xi in x.values()], cover[i], ubound[i]) if lbound[i] <= 0 else count_0([xi == cover[i] for xi in x.values()]) in range(lbound[i], ubound[i] + 1) for i in index_set(cover)])

def global_cardinality_1(x, cover, lbound, ubound):
    assert_(index_sets_agree(cover, lbound) and index_sets_agree(cover, ubound), 'global_cardinality: ' + 'cover has index sets ' + show_index_sets(cover) + ', lbound has index sets ' + show_index_sets(lbound) + ', and ubound has index sets ' + show_index_sets(lbound) + ', but they must have identical index sets', assert_(all([l <= 0 for l in array1d(lbound)]) and all([u >= 0 for u in array1d(ubound)]) or len(cover) == 0, 'global_cardinality_low_up: ' + 'lbound and ubound must allow a count of 0 when x is empty, or also be empty', True) if len(x) == 0 else fzn_global_cardinality_low_up(array1d(x), array1d(cover), array1d(lbound), array1d(ubound)))

def global_cardinality_low_up_35(x, cover, lbound, ubound):
    global_cardinality_1(x, cover, lbound, ubound)
k = 6
NURSE = set(range(1, k + 1))
m = 7
DAY = set(range(1, m + 1))
SHIFT = set(range(1, 3 + 1))
day = 1
night = 2
dayoff = 3
code = dict(zip(SHIFT, ['d', 'n', '-']))
o = 3
l = 2
u = 3
x = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(SHIFT), 'x' + str(key)) for key in product(NURSE, DAY)}
for n in NURSE:
    for d in range(1, m - 2 + 1):
        model.Add(impl_(model, and_(model, mors_lib_bool(model, model.Add(x[n, d] == night), model.Add(x[n, d] != night)), mors_lib_bool(model, model.Add(x[n, d + 1] == night), model.Add(x[n, d + 1] != night))), mors_lib_bool(model, model.Add(x[n, d + 2] == dayoff), model.Add(x[n, d + 2] != dayoff))) == True)
for n in NURSE:
    for d in range(1, m - 1 + 1):
        model.Add(impl_(model, mors_lib_bool(model, model.Add(x[n, d] == night), model.Add(x[n, d] != night)), mors_lib_bool(model, model.Add(x[n, d + 1] != day), model.Add(x[n, d + 1] == day))) == True)
for d in DAY:
    global_cardinality_low_up_35([x[n, d] for n in NURSE], [day, night], [o, l], [o, u])
solver = cp_model.CpSolver()
solution_printer = VarArraySolutionPrinter(NURSE, DAY, code, x, m)
status = solver.solve(model, solution_printer)