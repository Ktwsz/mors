import uuid
import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()
import mors_lib
mors_lib.model = model

class SolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, Seats, Guests, pos):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.Seats = Seats
        self.Guests = Guests
        self.pos = pos

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        for s in self.Seats:
            for g in self.Guests:
                if self.value(self.pos[g]) == s:
                    print(str(g) + ' ', end='')
        print('\n', end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def analyse_all_different_43(x):
    model.Add(True)

def all_different_42(x):
    analyse_all_different_43(array1d(x))
    ortools_all_different(array1d(x))

def alldifferent_41(x):
    all_different_42(array1d(x))

def let_in_2():
    p1 = pos[h1[h]]
    p2 = pos[h2[h]]
    same = bool2int(equiv_(mors_lib_bool(p1 <= 6, p1 > 6), mors_lib_bool(p2 <= 6, p2 > 6)))
    return mult(same, abs_(p1 - p2)) + mult(1 - same, abs_(13 - p1 - p2) + 1)
Guests = set(range(1, 12 + 1))
Seats = set(range(1, 12 + 1))
Hatreds = set(range(1, 5 + 1))
groom = 2
carol = 6
ed = 11
bride = 1
ted = 7
h1 = Array(Hatreds, Array([groom, carol, ed, bride, ted]))
clara = 12
bestman = 3
alice = 8
ron = 9
h2 = Array(Hatreds, Array([clara, bestman, ted, alice, ron]))
bob = 5
Males = {groom, bestman, bob, ted, ron, ed}
bridesmaid = 4
rona = 10
Females = {bride, bridesmaid, carol, alice, rona, clara}
pos = IntVarArray('pos', Guests, Seats)
alldifferent_41(pos)
for g in Males:
    model.Add(mod_(pos[g], 2) == 1)
for g in Females:
    model.Add(mod_(pos[g], 2) == 0)
model.Add(~in_(pos[ed], {1, 6, 7, 12}) == True)
model.Add(abs_(pos[bride] - pos[groom]) <= 1)
model.Add(mors_lib_bool(pos[bride] <= 6, pos[bride] > 6) == mors_lib_bool(pos[groom] <= 6, pos[groom] > 6))
model.maximize(sum(Array([let_in_2() for h in Hatreds])))
solver = cp_model.CpSolver()
solution_printer = SolutionPrinter(Seats, Guests, pos)
status = solver.solve(model, solution_printer)