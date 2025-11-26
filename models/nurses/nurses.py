import uuid
import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()
import mors_lib
mors_lib.model = model

class SolutionPrinter(cp_model.CpSolverSolutionCallback):

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
                print(self.code[self.value(self.x[n, d])] + ('\n' if d == self.m else ' '), end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def fzn_count_eq(x, y, c):
    ortools_count_eq(x, y, c)

def count_eq_3(x, y, c):
    fzn_count_eq(array1d(x), y, c)

def count_eq_2(x, y):

    def let_in_2():
        c = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(0, len(x) + 1)), 'c')
        count_eq_3(x, y, c)
        return c
    return let_in_2()

def fzn_global_cardinality_low_up_reif(x, cover, lbound, ubound):
    b = model.new_bool_var(str(uuid.uuid4()))
    model.Add(b == forall_(Array([in_(count_eq_2(x, cover[i]), range(lbound[i], ubound[i] + 1)) for i in index_set(cover)])))
    return b

def global_cardinality_1(x, cover, lbound, ubound):
    model.Add(assert_(index_sets_agree(cover, lbound) and index_sets_agree(cover, ubound), 'global_cardinality: ' + 'cover has index sets ' + show_index_sets(cover) + ', lbound has index sets ' + show_index_sets(lbound) + ', and ubound has index sets ' + show_index_sets(lbound) + ', but they must have identical index sets', assert_(all(Array([l <= 0 for l in array1d(lbound)])) and all(Array([u >= 0 for u in array1d(ubound)])) or len(cover) == 0, 'global_cardinality_low_up: ' + 'lbound and ubound must allow a count of 0 when x is empty, or also be empty', mors_lib_bool(True, False)) if len(x) == 0 else fzn_global_cardinality_low_up_reif(array1d(x), array1d(cover), array1d(lbound), array1d(ubound))) == True)

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
code = Array(SHIFT, Array(['d', 'n', '-']))
o = 3
l = 2
u = 3
x = IntVarArray('x', [NURSE, DAY], SHIFT)
for n in NURSE:
    for d in range(1, m - 2 + 1):
        model.add_implication(and_(mors_lib_bool(x[n, d] == night, x[n, d] != night), mors_lib_bool(x[n, d + 1] == night, x[n, d + 1] != night)), mors_lib_bool(x[n, d + 2] == dayoff, x[n, d + 2] != dayoff))
for n in NURSE:
    for d in range(1, m - 1 + 1):
        model.add_implication(mors_lib_bool(x[n, d] == night, x[n, d] != night), mors_lib_bool(x[n, d + 1] != day, x[n, d + 1] == day))
for d in DAY:
    global_cardinality_low_up_35(Array([x[n, d] for n in NURSE]), Array([day, night]), Array([o, l]), Array([o, u]))
solver = cp_model.CpSolver()
solution_printer = SolutionPrinter(NURSE, DAY, code, x, m)
status = solver.solve(model, solution_printer)