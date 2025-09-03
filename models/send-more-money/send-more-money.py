from ortools.sat.python import cp_model
from itertools import product
model = cp_model.CpModel()
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

def alldifferent_41(x):
    all_different_42(x)

def all_different_42(x):
    analyse_all_different_43(x) and model.add_all_different(x)

def analyse_all_different_43(x):
    True
solver = cp_model.CpSolver()
status = solver.solve(model)
print('   ' + format_40(S) + '' + format_40(E) + '' + format_40(N) + '' + format_40(D) + '\n', end='')
print('+  ' + format_40(M) + '' + format_40(O) + '' + format_40(R) + '' + format_40(E) + '\n', end='')
print('= ' + format_40(M) + '' + format_40(O) + '' + format_40(N) + '' + format_40(E) + '' + format_40(Y) + '\n', end='')