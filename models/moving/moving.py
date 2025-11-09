import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()

class VarArraySolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, start, end):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.start = start
        self.end = end

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        print('start = ' + (str([self.value(v) for v in self.start]) + ('\nend = ' + (str(self.value(self.end)) + '\n'))), end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def analyse_all_different_47(x):
    model.Add(True)

def disjunctive_44(s, d):
    ...

def all_different_46(x):
    analyse_all_different_47(array1d(x))
    all_different(model, array1d(x))

def cumulative_43(s, d, r, b):

    def let_in_1():
        mr = lb_array(r)
        mri = arg_min(arrayXd(r, [lb(r_i) for r_i in r.values()]))
        return all([True and (r[i] + mr > ub(b) or i == mri) for i in r.keys()])

    def let_in_1():
        mr = lb_array(r)
        mri = arg_min(arrayXd(r, [lb(r_i) for r_i in r.values()]))
        return all([True and (r[i] + mr > ub(b) or i == mri) for i in r.keys()])

    def let_in_1():
        mr = lb_array(r)
        mri = arg_min(arrayXd(r, [lb(r_i) for r_i in r.values()]))
        return all([True and (r[i] + mr > ub(b) or i == mri) for i in r.keys()])

    def let_in_1():
        mr = lb_array(r)
        mri = arg_min(arrayXd(r, [lb(r_i) for r_i in r.values()]))
        return all([True and (r[i] + mr > ub(b) or i == mri) for i in r.keys()])
    ...
    if len(s) >= 1:
        ...
        if let_in_1():
            for i in r.keys():
                model.Add(or_(model, mors_lib_bool(model, model.Add(d[i] == 0), model.Add(d[i] != 0)), mors_lib_bool(model, model.Add(r[i] <= b), model.Add(r[i] > b))) == True)
            if len(s) == 1:
                model.Add(True)
            elif all([True and d[i] == 1 for i in d.keys()]):
                all_different_46(s)
            else:
                disjunctive_44(s, d)
        else:
            fzn_cumulative(s, d, r, b)
    else:
        model.Add(True)
OBJECTS = set(range(1, 8 + 1))
duration = dict(zip(OBJECTS, [60, 45, 30, 30, 20, 15, 15, 15]))
handlers = dict(zip(OBJECTS, [3, 2, 2, 1, 2, 1, 1, 2]))
trolleys = dict(zip(OBJECTS, [2, 1, 2, 2, 2, 0, 0, 1]))
available_handlers = 4
available_trolleys = 3
available_time = 180
start = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(range(0, available_time + 1)), 'start' + str(key)) for key in OBJECTS}
end = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(0, available_time + 1)), 'end')
cumulative_43(start, duration, handlers, available_handlers)
cumulative_43(start, duration, trolleys, available_trolleys)
for o in OBJECTS:
    model.Add(start[o] + duration[o] <= end)
model.minimize(end)
solver = cp_model.CpSolver()
solution_printer = VarArraySolutionPrinter(start, end)
status = solver.solve(model, solution_printer)