import uuid
import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()
import mors_lib
mors_lib.model = model

class SolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, main, name, side, dessert, budget):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.main = main
        self.name = name
        self.side = side
        self.dessert = dessert
        self.budget = budget

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        print('main = ', end='')
        print(str(self.value(self.main[self.name])), end='')
        print(', side = ', end='')
        print(str(self.value(self.side[self.name])), end='')
        print(', dessert = ', end='')
        print(str(self.value(self.dessert[self.name])), end='')
        print(', cost = ', end='')
        print(str(self.value(self.budget)), end='')
        print('\n', end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def fzn_table_int(x, t):
    ortools_allowed_assignments(x, array1d(t))

def table_32(x, t):
    assert_(index_set_2of2(t) == index_set(x), 'The second dimension of the table must equal the number of variables ' + 'in the first argument')
    fzn_table_int(x, t)
min_energy = 3300
min_protein = 500
max_salt = 180
max_fat = 320
FOOD = set(range(1, 9 + 1))
icecream = 1
banana = 2
chocolatecake = 3
desserts = {icecream, banana, chocolatecake}
lasagna = 4
steak = 5
rice = 6
mains = {lasagna, steak, rice}
chips = 7
brocolli = 8
beans = 9
sides = {chips, brocolli, beans}
FEATURE = set(range(1, 6 + 1))
dd = Array([FOOD, FEATURE], Array([icecream, 1200, 50, 10, 120, 400, banana, 800, 120, 5, 20, 120, chocolatecake, 2500, 400, 20, 100, 600, lasagna, 3000, 200, 100, 250, 450, steak, 1800, 800, 50, 100, 1200, rice, 1200, 50, 5, 20, 100, chips, 2000, 50, 200, 200, 250, brocolli, 700, 100, 10, 10, 125, beans, 1900, 250, 60, 90, 150]))
main = IntVarArray('main', FEATURE, None)
side = IntVarArray('side', FEATURE, None)
dessert = IntVarArray('dessert', FEATURE, None)
budget = model.new_int_var(-4611686018427387, 4611686018427387, 'budget')
name = 1
energy = 2
protein = 3
salt = 4
fat = 5
cost = 6
model.Add(in_(main[name], mains) == True)
model.Add(in_(side[name], sides) == True)
model.Add(in_(dessert[name], desserts) == True)
table_32(main, dd)
table_32(side, dd)
table_32(dessert, dd)
model.Add(main[energy] + side[energy] + dessert[energy] >= min_energy)
model.Add(main[protein] + side[protein] + dessert[protein] >= min_protein)
model.Add(main[salt] + side[salt] + dessert[salt] <= max_salt)
model.Add(main[fat] + side[fat] + dessert[fat] <= max_fat)
model.Add(budget == main[cost] + side[cost] + dessert[cost])
model.minimize(budget)
solver = cp_model.CpSolver()
solution_printer = SolutionPrinter(main, name, side, dessert, budget)
status = solver.solve(model, solution_printer)