import uuid
import math
from ortools.sat.python import cp_model
from mors_lib import *
import mors_lib
model = cp_model.CpModel()
mors_lib.model = model

class SolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, end, JOB, TASK, digs, s, tasks):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.end = end
        self.JOB = JOB
        self.TASK = TASK
        self.digs = digs
        self.s = s
        self.tasks = tasks

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        print('end = ' + (str(self.value(self.end)) + '\n'), end='')
        for i in self.JOB:
            for j in self.TASK:
                print(show_int(self.digs, self.value(self.s[i, j])) + ' ' + ('\n' if j == self.tasks else ''), end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def no_overlap(s1, d1, s2, d2):
    model.add_bool_or(mors_lib_bool(s1 + d1 <= s2, s1 + d1 > s2), mors_lib_bool(s2 + d2 <= s1, s2 + d2 > s1))
jobs = 5
JOB = set(range(1, jobs + 1))
tasks = 5
TASK = set(range(1, tasks + 1))
d = Array([JOB, TASK], Array([1, 4, 5, 3, 6, 3, 2, 7, 1, 2, 4, 4, 4, 4, 4, 1, 1, 1, 6, 8, 7, 3, 2, 2, 1]))
total = sum(Array([d[i, j] for i in JOB for j in TASK]))
digs = math.ceil(math.log(float(total)))
s = IntVarArray('s', [JOB, TASK], range(0, total + 1))
end = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(0, total + 1)), 'end')
for i in JOB:
    for j in range(1, tasks - 1 + 1):
        model.Add(s[i, j] + d[i, j] <= s[i, j + 1])
    model.Add(s[i, tasks] + d[i, tasks] <= end)
for j in TASK:
    for i in JOB:
        for k in JOB:
            if i < k:
                no_overlap(s[i, j], d[i, j], s[k, j], d[k, j])
model.minimize(end)
solver = cp_model.CpSolver()
solution_printer = SolutionPrinter(end, JOB, TASK, digs, s, tasks)
status = solver.solve(model, solution_printer)