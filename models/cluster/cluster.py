import uuid
import math
from ortools.sat.python import cp_model
from mors_lib import *
import mors_lib
model = cp_model.CpModel()
mors_lib.model = model

class SolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, obj, x):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.obj = obj
        self.x = x

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        print('obj = ' + (str(self.value(self.obj)) + ('; x = ' + (str([self.value(v) for v in self.x]) + '\n'))), end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def max_5(x, y):

    def let_in_5():
        m = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(max(lb(x), lb(y)), max(ub(x), ub(y)) + 1)), 'm')
        int_max(x, y, m)
        return m
    return let_in_5()

def fzn_seq_precede_chain_int(X):

    def let_in_4():
        l = lb_array(X)
        u = ub_array(X)
        f = min(index_set(X))
        H = IntVarArray('H', index_set(X), range(l, u + 1))
        model.Add(H[f] <= 1)
        model.Add(H[f] == max_5(X[f], 0))
        for i in index_set(X) - {f}:
            model.Add(H[i] <= H[i - 1] + 1)
            model.Add(H[i] == max_5(X[i], H[i - 1]))
    if len(X) == 0:
        model.Add(True)
    else:
        let_in_4()

def seq_precede_chain(x):
    fzn_seq_precede_chain_int(x)

def min_6(x, y):

    def let_in_2():
        m = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(min(lb(x), lb(y)), min(ub(x), ub(y)) + 1)), 'm')
        int_min(x, y, m)
        m
    let_in_2()

def min_6(x, y):

    def let_in_2():
        m = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(min(lb(x), lb(y)), min(ub(x), ub(y)) + 1)), 'm')
        int_min(x, y, m)
        return m
    return let_in_2()

def fzn_value_precede_chain_int(T, X):

    def let_in_6():
        offset = min(index_set(T)) - 1
        l = lb_array(X)
        u = ub_array(X)
        p = Array([sum(Array([i - offset for i in index_set(T) if T[i] == j])) for j in range(l, u + 1)])
        Y = array1d(index_set(X), Array([p[X[i] - l + 1] for i in index_set(X)]))
        seq_precede_chain(Y)
    if len(T) == 0:
        model.Add(True)
    elif (min(index_set(T)) == 1 and all(Array([T[i] == i for i in index_set(T)]))) and max(T) == ub_array(X):
        seq_precede_chain(X)
    else:
        let_in_6()

def min_t(x):

    def let_in_3():
        m = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(lb_array(x), ub_array(x) + 1)), 'm')
        array_int_minimum(m, x)
        m
    if len(x) == 0:
        0
    elif len(x) == 1:
        x[1]
    elif len(x) == 2:
        min_6(x[1], x[2])
    else:
        let_in_3()

def min_t(x):

    def let_in_3():
        m = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(lb_array(x), ub_array(x) + 1)), 'm')
        array_int_minimum(m, x)
        return m
    return 0 if len(x) == 0 else x[1] if len(x) == 1 else min_6(x[1], x[2]) if len(x) == 2 else let_in_3()

def value_precede_chain_33(c, x):
    fzn_value_precede_chain_int(c, x)

def min_34(x):

    def let_in_1():
        xx = array1d(x)
        model.Add(len(x) >= 1)
        min_t(xx)
    let_in_1()

def min_34(x):

    def let_in_1():
        xx = array1d(x)
        model.Add(len(x) >= 1)
        return min_t(xx)
    return let_in_1()
n = 5
POINT = set(range(1, n + 1))
dist = Array([POINT, POINT], Array([0, 1, 2, 3, 4, 1, 0, 3, 2, 1, 2, 3, 0, 4, 1, 3, 2, 4, 0, 1, 4, 1, 1, 1, 0]))
maxdist = max(Array([dist[i, j] for i in POINT for j in POINT]))
k = 3
CLUSTER = set(range(1, k + 1))
maxdiam = 3
x = IntVarArray('x', POINT, CLUSTER)
obj = min_34(Array([dist[i, j] + maxdist * bool2int(mors_lib_bool(x[i] != x[j], x[i] == x[j])) for i in POINT for j in POINT if i < j]))
for i in POINT:
    for j in POINT:
        if i < j and x[i] == x[j]:
            model.Add(dist[i, j] <= maxdiam)
for i in range(1, k - 1 + 1):
    model.Add(min_34(Array([j for j in POINT if x[j] == i])) < min_34(Array([j for j in POINT if x[j] == i + 1])))
value_precede_chain_33(Array([i for i in range(1, k + 1)]), x)
model.maximize(obj)
solver = cp_model.CpSolver()
solution_printer = SolutionPrinter(obj, x)
status = solver.solve(model, solution_printer)