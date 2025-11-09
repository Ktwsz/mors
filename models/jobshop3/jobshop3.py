import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()

class VarArraySolutionPrinter(cp_model.CpSolverSolutionCallback):

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

def disjunctive_strict_31(s, d):
    return assert_(index_sets_agree(s, d), 'disjunctive: the array arguments must have identical index sets', ortools_disjunctive_strict(model, s, d))

def disjunctive_30(s, d):
    assert_(index_sets_agree(s, d), 'disjunctive: the array arguments must have identical index sets', disjunctive_strict_31(s, d) if lb_array(d) > 0 else ortools_disjunctive(model, s, d))
jobs = 5
JOB = set(range(1, jobs + 1))
tasks = 5
TASK = set(range(1, tasks + 1))
d = dict(zip(product(JOB, TASK), [1, 4, 5, 3, 6, 3, 2, 7, 1, 2, 4, 4, 4, 4, 4, 1, 1, 1, 6, 8, 7, 3, 2, 2, 1]))
total = sum([d[i, j] for i in JOB for j in TASK])
digs = math.ceil(math.log(float(total)))
s = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(range(0, total + 1)), 's' + str(key)) for key in product(JOB, TASK)}
end = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(0, total + 1)), 'end')
for i in JOB:
    for j in range(1, tasks - 1 + 1):
        model.Add(s[i, j] + d[i, j] <= s[i, j + 1])
    model.Add(s[i, tasks] + d[i, tasks] <= end)
for j in TASK:
    disjunctive_30([s[i, j] for i in JOB], [d[i, j] for i in JOB])
model.minimize(end)
solver = cp_model.CpSolver()
solution_printer = VarArraySolutionPrinter(end, JOB, TASK, digs, s, tasks)
status = solver.solve(model, solution_printer)