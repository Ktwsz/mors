import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()

class VarArraySolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, Seats, Guests):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.Seats = Seats
        self.Guests = Guests

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        for s in self.Seats:
            for g in self.Guests:
                if pos[g] == s:
                    print(str(g) + ' ', end='')
        print('\n', end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def analyse_all_different_43(x):
    model.Add(True)

def all_different_42(x):
    analyse_all_different_43(array1d(x))
    all_different(model, array1d(x))

def alldifferent_41(x):
    all_different_42(array1d(x))

def let_in_2():
    p1 = model.new_int_var_from_domain(cp_model.Domain.FromValues(Seats), 'p1')
    p2 = model.new_int_var_from_domain(cp_model.Domain.FromValues(Seats), 'p2')
    same = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(0, 1 + 1)), 'same')
    return mult(model, same, abs(p1 - p2)) + mult(model, 1 - same, abs(13 - p1 - p2) + 1)
Guests = set(range(1, 12 + 1))
Seats = set(range(1, 12 + 1))
Hatreds = set(range(1, 5 + 1))
groom = 2
carol = 6
ed = 11
bride = 1
ted = 7
h1 = dict(zip(Hatreds, [groom, carol, ed, bride, ted]))
clara = 12
bestman = 3
alice = 8
ron = 9
h2 = dict(zip(Hatreds, [clara, bestman, ted, alice, ron]))
bob = 5
Males = set({groom, bestman, bob, ted, ron, ed})
bridesmaid = 4
rona = 10
Females = set({bride, bridesmaid, carol, alice, rona, clara})
pos = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(Seats), 'pos' + str(key)) for key in Guests}
alldifferent_41(pos)
for g in Males:
    model.Add(mod_(model, pos[g], 2) == 1)
for g in Females:
    model.Add(mod_(model, pos[g], 2) == 0)
model.Add(~in_(model, pos[ed], {1, 6, 7, 12}) == True)
model.Add(abs_(model, pos[bride] - pos[groom]) <= 1)
model.Add(equiv_(model, mors_lib_bool(model, model.Add(pos[bride] <= 6), model.Add(pos[bride] > 6)), mors_lib_bool(model, model.Add(pos[groom] <= 6), model.Add(pos[groom] > 6))) == True)
model.maximize(sum([let_in_2() for h in Hatreds]))
solver = cp_model.CpSolver()
solution_printer = VarArraySolutionPrinter(Seats, Guests)
status = solver.solve(model, solution_printer)