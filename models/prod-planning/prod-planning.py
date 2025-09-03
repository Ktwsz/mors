from ortools.sat.python import cp_model
from itertools import product
model = cp_model.CpModel()
Products = set(range(1, 2 + 1))
profit = dict(zip(Products, [400, 450]))
Resources = set(range(1, 5 + 1))
capacity = dict(zip(Resources, [4000, 6, 2000, 500, 500]))
consumption = dict(zip(product(Products, Resources), [250, 2, 75, 100, 0, 200, 0, 150, 150, 75]))
mproducts = max([min([capacity[r] // consumption[p, r] for r in Resources if consumption[p, r] > 0]) for p in Products])
produce = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(range(0, mproducts + 1)), 'produce' + str(key)) for key in Products}
used = {key: model.new_int_var_from_domain(cp_model.Domain.FromValues(range(0, max(capacity) + 1)), 'used' + str(key)) for key in Resources}
for r in Resources:
    model.Add(used[r] == sum([consumption[p, r] * produce[p] for p in Products]))
for r in Resources:
    model.Add(used[r] <= capacity[r])
solver = cp_model.CpSolver()
status = solver.solve(model)