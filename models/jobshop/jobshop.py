import uuid
import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()
import mors_lib
mors_lib.model = model

class SolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, end, JOB, TASK, digs, s, last):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.end = end
        self.JOB = JOB
        self.TASK = TASK
        self.digs = digs
        self.s = s
        self.last = last

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        print('end = ' + (str(self.value(self.end)) + '\n'), end='')
        for i in self.JOB:
            for j in self.TASK:
                print(show_int(self.digs, self.value(self.s[i, j])) + ' ' + ('\n' if j == self.last else ''), end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count
JOB = set(range(1, max(range(1, 5 + 1)) + 1))
TASK = set(range(1, max(range(1, 5 + 1)) + 1))
last = max(TASK)
d = Array([JOB, TASK], Array([1, 4, 5, 3, 6, 3, 2, 7, 1, 2, 4, 4, 4, 4, 4, 1, 1, 1, 6, 8, 7, 3, 2, 2, 1]))
total = sum(Array([d[i, j] for i in JOB for j in TASK]))
digs = math.ceil(math.log(float(total)))
s = IntVarArray('s', [JOB, TASK], range(0, total + 1))
end = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(0, total + 1)), 'end')
for i in JOB:
    for j in TASK:
        if j < last:
            model.Add(s[i, j] + d[i, j] <= s[i, j + 1])
    model.Add(s[i, last] + d[i, last] <= end)
for j in TASK:
    for i in JOB:
        for k in JOB:
            if i < k:
                model.add_bool_or(mors_lib_bool(s[i, j] + d[i, j] <= s[k, j], s[i, j] + d[i, j] > s[k, j]), mors_lib_bool(s[k, j] + d[k, j] <= s[i, j], s[k, j] + d[k, j] > s[i, j]))
model.minimize(end)
solver = cp_model.CpSolver()
solution_printer = SolutionPrinter(end, JOB, TASK, digs, s, last)
status = solver.solve(model, solution_printer)