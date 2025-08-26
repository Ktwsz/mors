from ortools.sat.python import cp_model
from itertools import product
model = cp_model.CpModel()
Products = set(range(1, 2 + 1))
profit = dict(zip(Products, [400, 450]))
Resources = set(range(1, 5 + 1))
capacity = dict(zip(Resources, [4000, 6, 2000, 500, 500]))
consumption = dict(zip(product([Products, Resources], [250, 2, 75, 100, 0, 200, 0, 150, 150, 75])))
mproducts = max([min([capacity[r] // consumption[p, r] for r in Resources]) for p in Products])