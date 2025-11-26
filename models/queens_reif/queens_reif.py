import uuid
import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()
import mors_lib
mors_lib.model = model

class SolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, n, q):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.n = n
        self.q = q

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        for i in range(1, self.n + 1):
            for j in range(1, self.n + 1):
                print(('Q ' if self.value(self.q[i]) == j else '. ') + ('\n' if j == self.n else ''), end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def fzn_all_different_int_reif(x):
    b = model.new_bool_var(str(uuid.uuid4()))
    model.Add(b == forall_(Array([mors_lib_bool(x[i] != x[j], x[i] == x[j]) for i in index_set(x) for j in index_set(x) if i < j])))
    return b

def analyse_all_different_42(x):
    return mors_lib_bool(True, False)

def all_different_41(x):
    return and_(analyse_all_different_42(array1d(x)), fzn_all_different_int_reif(array1d(x)))

def alldifferent_40(x):
    return all_different_41(array1d(x))
n = 10
q = IntVarArray('q', range(1, n + 1), range(1, n + 1))
model.Add(mors_lib_bool(True, False) == alldifferent_40(q))
model.Add(mors_lib_bool(True, False) == alldifferent_40(Array([q[i] + i for i in range(1, n + 1)])))
model.Add(mors_lib_bool(True, False) == alldifferent_40(Array([q[i] - i for i in range(1, n + 1)])))
solver = cp_model.CpSolver()
solution_printer = SolutionPrinter(n, q)
status = solver.solve(model, solution_printer)