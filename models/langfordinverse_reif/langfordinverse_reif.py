import uuid
import math
from ortools.sat.python import cp_model
from mors_lib import *
import mors_lib
model = cp_model.CpModel()
mors_lib.model = model

class SolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, x, POS, y, m):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.x = x
        self.POS = POS
        self.y = y
        self.m = m

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        print(str([self.value(v) for v in self.x]), end='')
        print('\n', end='')
        for p in self.POS:
            print(str((self.value(self.y[p]) - 1) // self.m + 1) + ' ', end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def fzn_inverse_reif(f, invf):
    b = model.new_bool_var(str(uuid.uuid4()))
    model.Add(b == and_(forall_(Array([and_(in_(f[i], index_set(invf)), mors_lib_bool(invf[f[i]] == i, invf[f[i]] != i)) for i in index_set(f)])), forall_(Array([and_(in_(invf[j], index_set(f)), mors_lib_bool(f[invf[j]] == j, f[invf[j]] != j)) for j in index_set(invf)]))))
    return b

def analyse_all_different_16(x):
    return mors_lib_bool(True, False)

def inverse_42(f, invf):
    return and_(and_(analyse_all_different_16(f), analyse_all_different_16(invf)), fzn_inverse_reif(f, invf))
n = 10
DIG = set(range(1, n + 1))
m = 3
COPY = set(range(1, m + 1))
l = m * n
POS = set(range(1, l + 1))
x = IntVarArray('x', [DIG, COPY], POS)
DIGCOPY = set(range(1, l + 1))
y = IntVarArray('y', POS, DIGCOPY)
model.Add(mors_lib_bool(True, False) == inverse_42(Array([x[d, c] for d in DIG for c in COPY]), y))
for d in DIG:
    for c in range(1, m - 1 + 1):
        model.Add(x[d, c + 1] == x[d, c] + d + 1)
solver = cp_model.CpSolver()
solution_printer = SolutionPrinter(x, POS, y, m)
status = solver.solve(model, solution_printer)