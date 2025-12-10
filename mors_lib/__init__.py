import uuid
from itertools import product
from collections.abc import Iterable
from ortools.sat.python.cp_model import IntVar, Domain, Constraint
from ortools.sat.python import cp_model_helper
from multimethod import multimethod

model = None

import sys
import json

class JsonParams:
    if len(sys.argv) == 2:
        with open(sys.argv[1], 'r') as file:
            data_file = json.load(file)

@multimethod
def containts_intvar(v: tuple) -> bool:
    # if isinstance(idx, Iterable):
    #     for i in idx:
    #         if isinstance(i, IntVar):
    #             return True
    # elif isinstance(idx, IntVar):
    #     return True
    # return False
        for i in v:
            if containts_intvar(i):
                return True
        return False

@multimethod
def containts_intvar(v: IntVar) -> bool:
    return True

@multimethod
def containts_intvar(v: int) -> bool:
    return False

@multimethod
def match_idx(ix1: tuple, pattern: tuple) -> bool:
    for i, p in zip(ix1, pattern):
        if not match_idx(i, p):
            return False
    return True

@multimethod
def match_idx(idx: int, pattern: IntVar) -> bool:
    return True

@multimethod
def match_idx(idx: int, pattern: int) -> bool:
    return idx == pattern

# @multimethod
# def strip_idx(idx: tuple, pattern: tuple) -> tuple[int] | int:
#     new_idx = []
#     for i, p in zip(idx, pattern):
#         if not containts_intvar(p):
#             continue
#
#         new_idx.append(i)
#
#     if len(new_idx) == 1:
#         return new_idx
#
#     return tuple(new_idx)
#
# @multimethod
# def strip_idx(idx: int, pattern: IntVar) -> int:
#     return idx

def intvar_dims(idx: tuple[int | IntVar]) -> tuple[int]:
    dims = []
    for dim, i in enumerate(idx):
        if containts_intvar(i):
            dims.append(dim)

    return tuple(dims)

def flatten_nd(index: tuple[IntVar], shapes: tuple[int], base_ix: tuple[int]):
    strides = [1]
    for size in reversed(shapes[1:]):
        strides.insert(0, strides[0] * size)

    return sum( (i - base) * s for i, s, base in zip(index, strides, base_ix))

class Array:
    def __init__(self, indexes, values=None):
        if values is None:
            self.construct(list(range(1, len(indexes)+1)), indexes)
            return

        self.construct(indexes, values)

    def construct(self, indexes, values):
        if isinstance(indexes, list) and isinstance(indexes[0], Iterable):
            self.values = dict(zip(product(*indexes), values))
        else:
            self.values = dict(zip(indexes, values))

    def slice(self, pattern):
        return [
            # (strip_idx(idx, pattern), value)
            (idx, value)
            for idx, value in self.values.items()
            if match_idx(idx, pattern)
        ]


    def __add__(self, rhs):
        return Array(list(self.values.values()) + list(rhs.values.values()))

    def __iter__(self):
        return iter(self.values.values())

    def __len__(self):
        return len(self.values)

    def __getitem__(self, idx): 
        if containts_intvar(idx):
            return access(self, idx)
        return self.values[idx]

def load_from_json(name):
    if JsonParams.data_file is None:
        raise AssertionError("Must specify the JSON param file")
    return JsonParams.data_file[name]

def coerce_set(s):
    if isinstance(s, dict):
        s = s["set"]

    result = set()
    for v in s:
        if isinstance(v, list) or isinstance(v, tuple):
            for i in range(v[0], v[1] + 1):
                result.add(i)
            continue

        result.add(v)
    return result

def load_array_from_json(name, *dims):
    if JsonParams.data_file is None:
        raise AssertionError("Must specify the JSON param file")

    arr = JsonParams.data_file[name]
    for _ in range(len(dims)-1):
        new_arr = []
        for a in arr:
            new_arr.extend(a)
        arr = new_arr

    is_set = lambda x: isinstance(x, list) or (isinstance(x, dict) and "set" in x)
    if len(arr) > 0 and is_set(arr[0]):
        for i in range(len(arr)):
            arr[i] = coerce_set(arr[i])

    if len(dims) > 1:
        return Array(list(dims), arr)
    return Array(dims[0], arr)

def load_set_from_json(name):
    if JsonParams.data_file is None:
        raise AssertionError("Must specify the JSON param file")

    return coerce_set(JsonParams.data_file[name])

def values_from_flat_interval(interval):
    values = []
    for i in range(0, len(interval), 2):
        values.extend(list(range(interval[i], interval[i+1]+1)))
    return values

@multimethod
def access(arr: Array, idx: tuple): 
    def get_new_domain(arr):
        domain = Domain.FromValues([])
        for v in arr:
            if isinstance(v, IntVar):
                add_domain = Domain.FromFlatIntervals(v.proto.domain)
            else:
                add_domain = Domain.FromValues([v])
            domain = domain.union_with(add_domain)
        return domain

    key_values = arr.slice(idx)
    dims = intvar_dims(idx)
    if len(dims) == 1:
        dim = dims[0]
        base = min([key[dim] for key, _ in key_values])
        index = idx[dim] - base
        expressions = [v for _, v in key_values]
        element = model.new_int_var_from_domain(get_new_domain(expressions), str(uuid.uuid4()))

        model.AddElement(index, expressions, element)
        return element

    idx_intvar = tuple([idx[dim] for dim in dims])

    base_ix = [
        min([key[dim] for key, _ in key_values]) 
        for dim in dims
    ]
    shapes = [
        max([key[dim] for key, _ in key_values]) - min([key[dim] for key, _ in key_values]) + 1
        for dim in dims
    ]
    index = flatten_nd(idx_intvar, shapes, base_ix)
    expressions = [v for _, v in key_values]
    index_var = model.new_int_var(0, len(expressions), str(uuid.uuid4()))
    model.Add(index_var == index)
    element = model.new_int_var_from_domain(get_new_domain(expressions), str(uuid.uuid4()))

    model.AddElement(index_var, expressions, element)
    return element

@multimethod
def access(arr: Array, idx: IntVar): 
    def get_new_domain(arr):
        domain = Domain.FromValues([])
        for v in arr:
            if isinstance(v, IntVar):
                add_domain = Domain.FromFlatIntervals(v.proto.domain)
            else:
                add_domain = Domain.FromValues([v])
            domain = domain.union_with(add_domain)
        return domain

    key_values = arr.slice(idx)

    base = min([key for key, _ in key_values])
    index = idx - base
    expressions = [v for _, v in key_values]
    element = model.new_int_var_from_domain(get_new_domain(expressions), str(uuid.uuid4()))

    model.AddElement(index, expressions, element)
    return element

def int_affine_domain(expr):
    new_values = []
    for v in values_from_flat_interval(expr.expression.proto.domain):
        new_values.append(v * expr.coefficient + expr.offset)
        
    return Domain.from_values(new_values)

def get_domain(x):
    if isinstance(x, IntVar):
        return Domain.from_flat_intervals(x.proto.domain)
    if isinstance(x, cp_model_helper.IntAffine):
        return int_affine_domain(x)
    if isinstance(x, int):
        return Domain.from_values([x])
    return x

def ub(x):
    return get_domain(x).max()

def lb(x):
    return get_domain(x).min()

def lb_array(x: Array):
    return min([lb(v) for v in x])

def ub_array(x: Array):
    return max([ub(v) for v in x])

def dom_array(x: Array):
    domains = [get_domain(v) for v in x]
    new_domain = domains[0]
    for d in domains[1:]:
        new_domain = new_domain.union_with(d)
    return set(values_from_flat_interval(new_domain.flattened_intervals()))

def dom(x):
    if isinstance(x, int):
        return {x}
    return set(values_from_flat_interval(get_domain(x).flattened_intervals()))

def show_index_sets(idx):
    return str(idx)


def array1d_1arg(x: Array):
    return Array(x.values.values())

def array1d(S, x = None):
    if x is None:
        return array1d_1arg(S)

    return Array(S, x.values.values())

def array2d(S1, S2, x: Array):
    return Array([S1, S2], x)

def arrayXd(x: Array, y: Array):
    return Array(x.values.keys(), y.values.values())

def index_set(x: Array):
    return list(x.values.keys())

def index_sets_agree(x: Array, y: Array):
    return x.values.keys() == y.values.keys()

def index_set_2of2(x: Array):
    return list(set([
        k[1]
        for k in x.values.keys()
    ]))

def arg_min(x: Array):
    return min(x.values.keys(), key=lambda k: x[k])

def mult(a, b):
    target = model.new_int_var(-46116860, 46116860, str(uuid.uuid4()))
    model.add_multiplication_equality(target, a, b)

    return target

def assert_(b, msg, x=None):
    assert b, msg
    return x

def abort(message):
    assert False, message

def symdiff_(a, b):
    return (a - b) | (b - a)

def IntVarArray(id, indexes, domain=None):
    if isinstance(indexes, list) and isinstance(indexes[0], Iterable):
        keys = product(*indexes)
    else:
        keys = indexes

    if domain is None:
        return Array(indexes, [model.new_int_var(-46116860,46116860, id + str(key)) for key in keys])

    return Array(indexes, [model.new_int_var_from_domain(Domain.FromValues(domain), id + str(key)) for key in keys])

def BoolVarArray(id, indexes):
    if isinstance(indexes, list) and isinstance(indexes[0], Iterable):
        keys = product(*indexes)
    else:
        keys = indexes

    return Array(indexes, [model.new_bool_var(id + str(key)) for key in keys])

def array_int_minimum(m, x: Array):
    model.add_min_equality(m, list(x.values.values()))

def array_int_maximum(m, x: Array):
    model.add_max_equality(m, list(x.values.values()))

def has_bounds(x):
    if isinstance(x, int):
        return True
    domain = x.proto.domain
    return domain[0] > -46116860 and domain[-1] < 46116860

def containts_linear_expr(v: Array):
    for var in v:
        if not isinstance(var, IntVar):
            return True
    return False


def ortools_all_different(v: Array):
    if not containts_linear_expr(v):
        model.add_all_different(v.values.values())

    vars = []
    for expr in v:
        vars.append(model.new_int_var(-46116860,46116860, str(uuid.uuid4())))
        model.add(vars[-1] == expr)
    model.add_all_different(vars)

def ortools_allowed_assignments(a: Array, b: Array):
    n = len(a)
    b_values = list(b.values.values())
    b = [ b_values[i:i+n] for i in range(0, len(b_values), n) ]

    model.add_allowed_assignments(a.values.values(), b)

def make_intervals(starts: Array, sizes: Array):
    intervals = []
    for start, size in zip(starts, sizes):
        start_domain = Domain.FromFlatIntervals(start.proto.domain)

        if isinstance(size, int):
            size_domain = Domain.FromValues([size])
        else:
            size_domain = Domain.FromFlatIntervals(size.proto.domain)

        end_var = model.new_int_var_from_domain(start_domain.addition_with(size_domain), str(uuid.uuid4()))
        intervals.append(model.new_interval_var(start, size, end_var, str(uuid.uuid4())))

    return intervals

def ortools_cumulative(starts: Array, sizes: Array, demands: Array, b):
    intervals = make_intervals(starts, sizes)

    model.add_cumulative(intervals, demands.values.values(), b)

def ortools_disjunctive_strict(starts: Array, sizes: Array):
    model.add_no_overlap(make_intervals(starts, sizes))

def ortools_circuit(x: Array, min_index, is_subcircuit=False):
    size = len(x)
    max_index = min_index + size - 1
     
    arcs = []
    for index, var in enumerate(x):
        domain = Domain.FromFlatIntervals(var.proto.domain)

        if is_subcircuit:
            index_domain = Domain.from_values(range(min_index, max_index + 1))
        else:
            index_domain = Domain.from_values(range(min_index, index - 1)).union_with(Domain.from_values(range(index + 1, max_index + 1)))

        domain = domain.interection_with(index_domain)

        for i in range(0, len(domain.proto.domain), 2):
            closed_interval = [domain.proto.domain[i], domain.proto.domain[i+1]]
            for value in range(closed_interval[0], closed_interval[1] + 1):
                literal = model.new_bool_var(str(uuid.uuid4()))
                arcs.append((index, value, literal))
                model.add(var == value).only_enforce_if(literal)
                model.add(var != value).only_enforce_if(~literal)

    model.add_circuit(arcs)

def ortools_count_eq(counts: Array, var, target):
    vars = []
    for count in counts:
        vars.append(mors_lib_bool(var == count, var != count))

    model.add(sum(vars) == target)

def ortools_count_eq_cst(counts: Array, value, target):
    ortools_count_eq(counts, value, target)

def ortools_diffn(x: Array, y: Array, dx: Array, dy: Array):
    x_intervals = make_intervals(x, dx)
    y_intervals = make_intervals(y, dy)

    model.add_no_overlap_2d(x_intervals, y_intervals)

def ortools_diffn_nonstrict(x: Array, y: Array, dx: Array, dy: Array):
    ortools_diffn(x, y, dx, dy)

def ortools_network_flow_cost(arcs: Array, nodes, base_node, flow, weights=None, cost=None):
    num_arcs = len(arcs) // 2
    num_nodes = len(nodes)

    flows_per_node = [[] for _ in range(num_nodes)]
    coeffs_per_node = [[] for _ in range(num_nodes)]

    for arc in range(num_arcs):
        tail = arcs[2 * arc] - base_node
        head = arcs[2 * arc + 1] - base_node
        if tail == head:
            continue

        flows_per_node[tail].append(flow[arc])
        coeffs_per_node[tail].append(1)
        flows_per_node[head].append(flow[arc])
        coeffs_per_node[head].append(-1)

    for node in range(num_nodes):
        model.Add(sum([ var * coeff for var, coeff in zip(flows_per_node[node], coeffs_per_node[node])]) == num_nodes)

    if weights is not None and cost is not None:
        model.Add(sum([
            flow[arc] * weights[arc]
            for arc in range(num_arcs)
            if weights[arc] != 0
        ]) == cost)


def ortools_network_flow(arcs: Array, nodes, base_node, flow):
    ortools_network_flow_cost(arcs, nodes, base_node, flow)

def ortools_regular(transition_expressions, num_states, num_values, d, starting_state, final_states):
    transition_triplets = []
    count = 0

    for i in range(1, num_state+1):
        for j in range(1, num_values+1):
            next = d[count]
            count += 1

            if next == 0:
                continue

            transition_triplets.append((i, j, next))

    model.add_automaton(transition_expressions, starting_state, final_states, transition_triplets)

def ortools_disjunctive(starts: Array, sizes: Array):
    model.add_cumualtive(make_intervals(starts, sizes), [1 for _ in range(len(starts))], 1)


def ortools_inverse(f: Array, invf: Array, base_direct, base_inverse):
    f = list(f.values.values())
    invf = list(invf.values.values())

    end_direct = base_direct + len(f)
    end_inverse = base_inverse + len(f)

    if base_direct == base_inverse:
        direct_variables = list(range(base_direct)) + f
        inverse_variables = list(range(base_direct)) + invf
    elif base_direct > base_inverse:
        offset = base_direct - base_inverse
        direct_variables = list(range(base_inverse)) + list(range(end_inverse, end_inverse+offset)) + f 
        inverse_variables = list(range(base_inverse)) + invf + list(range(base_inverse, base_inverse + offset))
    else:
        offset = base_inverse - base_direct 
        direct_variables = list(range(base_direct)) + f + list(range(base_direct, base_direct + offset))
        inverse_variables = list(range(base_direct)) + list(range(end_direct, end_direct + offset)) + invf

    model.add_inverse(direct_variables, inverse_variables)

def int_min(a, b, m):
    min_bool = mors_lib_bool(a < b, a >= b)
    model.Add(m == a).only_enforce_if(min_bool)
    model.Add(m == b).only_enforce_if(~min_bool)
    return m

def mors_lib_bool(c1, c2):
    if not isinstance(c1, Constraint):
        c1 = model.add(c1)
    if not isinstance(c2, Constraint):
        c2 = model.add(c2)

    b = model.new_bool_var(str(uuid.uuid4()))
    c1.only_enforce_if(b)
    c2.only_enforce_if(~b)

    return b

def and_(a_bool, b_bool):
    c1 = model.add_bool_and(a_bool, b_bool)
    c2 = model.add_bool_or(~a_bool, ~b_bool)

    return mors_lib_bool(c1, c2)


def or_(a_bool, b_bool):
    c1 = model.add_bool_or(a_bool, b_bool)
    c2 = model.add_bool_and(~a_bool, ~b_bool)

    return mors_lib_bool(c1, c2)

def xor_(a_bool, b_bool):
    return ~equiv(a_bool, b_bool)

def forall_(bools):
    c1 = model.add_bool_and(bools)
    c2 = model.add_bool_or([~b for b in bools])

    return mors_lib_bool(c1, c2)

def impl_(a_bool, b_bool):
    c1 = model.add_implication(a_bool, b_bool)
    c2 = model.add_bool_and(a_bool, ~b_bool)

    return mors_lib_bool(c1, c2)

def not_(a):
    return a[1], a[0]

def abs_(a):
    target = model.new_int_var(-46116860, 46116860, str(uuid.uuid4()))

    model.add_abs_equality(target, a)
    return target

def mod_(a, b):
    target = model.new_int_var(-46116860, 46116860, str(uuid.uuid4()))

    model.add_modulo_equality(target, a, b)
    return target

def equiv_(a, b):
    return and_(impl_(a, b), impl_(b, a))

def in_(var, arr):
    bools = [model.new_bool_var(str(uuid.uuid4())) for _ in range(len(arr))]

    for value, b_ in zip(arr, bools):
        model.Add(var == value).only_enforce_if(b_)
        model.Add(var != value).only_enforce_if(~b_)

    return mors_lib_bool(model.add_bool_or(bools), model.add_bool_and([~b for b in bools]))

def bool2int(a_bool):
    b = model.new_int_var(0, 1, str(uuid.uuid4()))
    model.Add(b == 1).only_enforce_if(a_bool)
    model.Add(b == 0).only_enforce_if(~a_bool)

    return b

def show_int(a, b):
    return str(b).rjust(a) if a > 0 else str(b).ljust(a)