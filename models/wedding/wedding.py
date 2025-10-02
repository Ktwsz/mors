import math
from ortools.sat.python import cp_model
import mors_lib
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
                if fix(pos[g]) == s:
                    print(str(g) + ' ', end='')
        print('\n', end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def analyse_all_different_43(x):
    return True

def all_different_42(x):
    return analyse_all_different_43(x) and model.add_all_different(x)

def alldifferent_41(x):
    return all_different_42(x)
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
p1 = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(Seats), 'p1' + str(key)) for key in Hatreds}
p2 = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(Seats), 'p2' + str(key)) for key in Hatreds}
sameside = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(range(0, 1 + 1)), 'sameside' + str(key)) for key in Hatreds}
cost = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(Seats), 'cost' + str(key)) for key in Hatreds}
alldifferent_41(pos)
for g in Males:
    model.Add(mors_lib.mod_(model, pos[g], 2) == 1)
for g in Females:
    model.Add(mors_lib.mod_(model, pos[g], 2) == 0)
model.Add(~mors_lib.in_(model, pos[ed], {1, 6, 7, 12}) == True)
model.Add(mors_lib.abs_(model, pos[bride] - pos[groom]) <= 1)
model.Add(mors_lib.equiv_(model, mors_lib.b(model, model.Add(pos[bride] <= 6), model.Add(pos[bride] > 6)), mors_lib.b(model, model.Add(pos[groom] <= 6), model.Add(pos[groom] > 6))) == True)
for h in Hatreds:
    model.Add(p1[h] == pos[h1[h]])
    model.Add(p2[h] == pos[h2[h]])
    model.Add(sameside[h] == mors_lib.bool2int(model, mors_lib.equiv_(model, mors_lib.b(model, model.Add(p1[h] <= 6), model.Add(p1[h] > 6)), mors_lib.b(model, model.Add(p2[h] <= 6), model.Add(p2[h] > 6)))))
    model.Add(cost[h] == sameside[h] * mors_lib.abs_(model, p1[h] - p2[h]) + (1 - sameside[h]) * (mors_lib.abs_(model, 13 - p1[h] - p2[h]) + 1))
model.maximize(sum([cost[h] for h in Hatreds]))
solver = cp_model.CpSolver()
solution_printer = VarArraySolutionPrinter(Seats, Guests)
status = solver.solve(model, solution_printer)