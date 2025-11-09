import math
from ortools.sat.python import cp_model
from mors_lib import *
from itertools import product
model = cp_model.CpModel()

class VarArraySolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, PuzzleRange, digs, puzzle):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.PuzzleRange = PuzzleRange
        self.digs = digs
        self.puzzle = puzzle

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        for i in self.PuzzleRange:
            for j in self.PuzzleRange:
                print(show_int(self.digs, self.value(self.puzzle[i, j])) + ' ' + (' ' if j % S == 0 else '') + ((('\n\n' if i % S == 0 else '\n') if i != N else '') if j == N else ''), end='')
        print('\n', end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def analyse_all_different_42(x):
    model.Add(True)

def all_different_41(x):
    analyse_all_different_42(array1d(x))
    all_different(model, array1d(x))

def alldifferent_40(x):
    all_different_41(array1d(x))
S = 3
N = S * S
digs = math.ceil(math.log(float(N)))
PuzzleRange = set(range(1, N + 1))
SubSquareRange = set(range(1, S + 1))
start = dict(zip(product(range(1, N + 1), range(1, N + 1)), [0, 6, 8, 4, 0, 1, 0, 7, 0, 0, 0, 0, 0, 8, 5, 0, 3, 0, 0, 2, 6, 8, 0, 9, 0, 4, 0, 0, 0, 7, 0, 0, 0, 9, 0, 0, 0, 5, 0, 1, 0, 6, 3, 2, 0, 0, 4, 0, 6, 1, 0, 0, 0, 0, 0, 3, 0, 2, 0, 7, 6, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]))
puzzle = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(PuzzleRange), 'puzzle' + str(key)) for key in product(range(1, N + 1), range(1, N + 1))}
for i in PuzzleRange:
    for j in PuzzleRange:
        if start[i, j] > 0:
            model.Add(puzzle[i, j] == start[i, j])
        else:
            model.Add(True)
for i in PuzzleRange:
    alldifferent_40([puzzle[i, j] for j in PuzzleRange])
for j in PuzzleRange:
    alldifferent_40([puzzle[i, j] for i in PuzzleRange])
for a in SubSquareRange:
    for o in SubSquareRange:
        alldifferent_40([puzzle[(a - 1) * S + a1, (o - 1) * S + o1] for a1 in SubSquareRange for o1 in SubSquareRange])
solver = cp_model.CpSolver()
solution_printer = VarArraySolutionPrinter(PuzzleRange, digs, puzzle)
status = solver.solve(model, solution_printer)