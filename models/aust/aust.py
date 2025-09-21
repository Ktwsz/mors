import math
from ortools.sat.python import cp_model
import mors_lib
from itertools import product
model = cp_model.CpModel()

class VarArraySolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, wa, nt, sa, q, nsw, v, t):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.wa = wa
        self.nt = nt
        self.sa = sa
        self.q = q
        self.nsw = nsw
        self.v = v
        self.t = t

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        print('wa=' + (str(self.value(self.wa)) + ''), end='')
        print(' nt=' + (str(self.value(self.nt)) + ''), end='')
        print(' sa=' + (str(self.value(self.sa)) + '\n'), end='')
        print('q=' + (str(self.value(self.q)) + ''), end='')
        print(' nsw=' + (str(self.value(self.nsw)) + ''), end='')
        print(' v=' + (str(self.value(self.v)) + '\n'), end='')
        print('t=' + (str(self.value(self.t)) + '\n'), end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count
nc = 4
wa = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(1, nc + 1)), 'wa')
nt = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(1, nc + 1)), 'nt')
sa = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(1, nc + 1)), 'sa')
q = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(1, nc + 1)), 'q')
nsw = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(1, nc + 1)), 'nsw')
v = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(1, nc + 1)), 'v')
t = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(1, nc + 1)), 't')
model.Add(wa != nt)
model.Add(wa != sa)
model.Add(nt != sa)
model.Add(nt != q)
model.Add(sa != q)
model.Add(sa != nsw)
model.Add(sa != v)
model.Add(q != nsw)
model.Add(nsw != v)
solver = cp_model.CpSolver()
solution_printer = VarArraySolutionPrinter(wa, nt, sa, q, nsw, v, t)
status = solver.solve(model, solution_printer)