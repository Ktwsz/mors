from ortools.sat.python import cp_model
from itertools import product
model = cp_model.CpModel()

class VarArraySolutionPrinter(cp_model.CpSolverSolutionCallback):

    def __init__(self, S, E, N, D, M, O, R, Y):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0
        self.S = S
        self.E = E
        self.N = N
        self.D = D
        self.M = M
        self.O = O
        self.R = R
        self.Y = Y

    def on_solution_callback(self) -> None:
        self.__solution_count += 1
        print('   ' + str(self.value(self.S)) + '' + str(self.value(self.E)) + '' + str(self.value(self.N)) + '' + str(self.value(self.D)) + '\n', end='')
        print('+  ' + str(self.value(self.M)) + '' + str(self.value(self.O)) + '' + str(self.value(self.R)) + '' + str(self.value(self.E)) + '\n', end='')
        print('= ' + str(self.value(self.M)) + '' + str(self.value(self.O)) + '' + str(self.value(self.N)) + '' + str(self.value(self.E)) + '' + str(self.value(self.Y)) + '\n', end='')

    @property
    def solution_count(self) -> int:
        return self.__solution_count

def analyse_all_different_43(x):
    return True

def all_different_42(x):
    return analyse_all_different_43(x) and model.add_all_different(x)

def alldifferent_41(x):
    return all_different_42(x)
S = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(1, 9 + 1)), 'S')
E = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(0, 9 + 1)), 'E')
N = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(0, 9 + 1)), 'N')
D = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(0, 9 + 1)), 'D')
M = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(1, 9 + 1)), 'M')
O = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(0, 9 + 1)), 'O')
R = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(0, 9 + 1)), 'R')
Y = model.new_int_var_from_domain(cp_model.Domain.FromValues(range(0, 9 + 1)), 'Y')
model.Add(1000 * S + 100 * E + 10 * N + D + 1000 * M + 100 * O + 10 * R + E == 10000 * M + 1000 * O + 100 * N + 10 * E + Y)
alldifferent_41([S, E, N, D, M, O, R, Y])
solver = cp_model.CpSolver()
solution_printer = VarArraySolutionPrinter(S, E, N, D, M, O, R, Y)
status = solver.solve(model, solution_printer)