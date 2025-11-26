import uuid
import math
from ortools.sat.python import cp_model
from mors_lib import *
import mors_lib
model = cp_model.CpModel()
mors_lib.model = model

class SolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, s):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.s = s

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        print(str([self.value(v) for v in self.s]), end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def fzn_disjunctive_strict_reif(s, d):
    b = model.new_bool_var(str(uuid.uuid4()))
    model.Add(b == and_(forall_(Array([mors_lib_bool(d[i] >= 0, d[i] < 0) for i in index_set(d)])), forall_(Array([or_(mors_lib_bool(s[i] + d[i] <= s[j], s[i] + d[i] > s[j]), mors_lib_bool(s[j] + d[j] <= s[i], s[j] + d[j] > s[i])) for i in index_set(d) for j in index_set(d) if i < j]))))
    return b

def max_15(x, y):

    def let_in_7():
        m = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(max(lb(x), lb(y)), max(ub(x), ub(y)) + 1)), 'm')
        int_max(x, y, m)
        return m
    return let_in_7()

def disjunctive_strict_42(s, d):
    return assert_(index_sets_agree(s, d), 'disjunctive: the array arguments must have identical index sets', fzn_disjunctive_strict_reif(s, d))

def fzn_disjunctive_reif(s, d):
    b = model.new_bool_var(str(uuid.uuid4()))
    model.Add(b == and_(forall_(Array([mors_lib_bool(d[i] >= 0, d[i] < 0) for i in index_set(d)])), forall_(Array([or_(or_(or_(mors_lib_bool(d[i] == 0, d[i] != 0), mors_lib_bool(d[j] == 0, d[j] != 0)), mors_lib_bool(s[i] + d[i] <= s[j], s[i] + d[i] > s[j])), mors_lib_bool(s[j] + d[j] <= s[i], s[j] + d[j] > s[i])) for i in index_set(d) for j in index_set(d) if i < j]))))
    return b

def analyse_all_different_44(x):
    model.Add(True)

def max_t(x):

    def let_in_8():
        m = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(lb_array(x), ub_array(x) + 1)), 'm')
        array_int_maximum(m, x)
        return m
    return 0 if len(x) == 0 else x[min(index_set(x))] if len(x) == 1 else max_15(x[1], x[2]) if len(x) == 2 else let_in_8()

def disjunctive_41(s, d):
    model.Add(assert_(index_sets_agree(s, d), 'disjunctive: the array arguments must have identical index sets', disjunctive_strict_42(s, d) if lb_array(d) > 0 else fzn_disjunctive_reif(s, d)) == True)

def all_different_43(x):
    analyse_all_different_44(array1d(x))
    ortools_all_different(array1d(x))

def max_39(x):

    def let_in_6():
        xx = array1d(x)
        model.Add(len(x) >= 1)
        return max_t(xx)
    return let_in_6()

def cumulative_40(s, d, r, b):

    def let_in_1():
        mr = lb_array(r)
        mri = arg_min(arrayXd(r, Array([lb(r_i) for r_i in r])))
        return all(Array([True and (r[i] + mr > ub(b) or i == mri) for i in index_set(r)]))
    assert_(index_set(s) == index_set(d) and index_set(s) == index_set(r), 'cumulative: the 3 array arguments must have identical index sets')
    if len(s) >= 1:
        assert_(lb_array(d) >= 0 and lb_array(r) >= 0, 'cumulative: durations and resource usages must be non-negative')
        if let_in_1():
            for i in index_set(r):
                model.add_bool_or(mors_lib_bool(d[i] == 0, d[i] != 0), mors_lib_bool(r[i] <= b, r[i] > b))
            if len(s) == 1:
                model.Add(True)
            elif all(Array([True and d[i] == 1 for i in index_set(d)])):
                all_different_43(s)
            else:
                disjunctive_41(s, d)
        else:
            ortools_cumulative(s, d, r, b)
    else:
        model.Add(True)
n = 5
TASK = set(range(1, n + 1))
d = Array(TASK, Array([3, 5, 6, 2, 3]))
m = 2
RESOURCE = set(range(1, m + 1))
L = Array(RESOURCE, Array([3, 4]))
res = Array([RESOURCE, TASK], Array([1, 2, 2, 2, 1, 3, 1, 2, 2, 1]))
l = 2
PREC = set(range(1, l + 1))
pre = Array([PREC, range(1, 2 + 1)], Array([1, 5, 5, 2]))
maxt = 20
TIME = set(range(0, maxt + 1))
s = IntVarArray('s', TASK, TIME)
for p in PREC:
    model.Add(s[pre[p, 1]] + d[pre[p, 1]] <= s[pre[p, 2]])
for r in RESOURCE:
    cumulative_40(s, d, Array([res[r, t] for t in TASK]), L[r])
model.minimize(max_39(Array([s[t] + d[t] for t in TASK])))
solver = cp_model.CpSolver()
solution_printer = SolutionPrinter(s)
status = solver.solve(model, solution_printer)