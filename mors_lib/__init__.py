import uuid
from itertools import product
from collections.abc import Iterable
from ortools.sat.python.cp_model import IntVar, Domain

def ub(x):
    if type(x) == IntVar: # TODO - linear constraints also
        return x.proto.domain[-1]
    return x

def lb(x):
    if type(x) == IntVar: # TODO - linear constraints also
        return x.proto.domain[0]
    return x

def lb_array(x):
    if type(x) == dict:
        return min([lb(v) for v in x.values()])

    return min([lb(v) for v in x])

def ub_array(x):
    if type(x) == dict:
        return max([ub(v) for v in x.values()])

    return max([ub(v) for v in x])

def array1d_1arg(x):
    if type(x) == dict:
        x = x.values()
    return dict(zip(range(1, len(x) + 1), x))

def array1d(S, x = None):
    if x is None:
        return array1d_1arg(S)

    if type(x) == dict:
        x = x.values()
    return dict(zip(S, x))

def array2d(S1, S2, x):
    if type(x) == dict:
        x = x.values()
    return dict(zip(product(S1, S2), x))

def arrayXd(x, y):
    return dict(zip(x.keys(), y))

def index_set(x):
    if type(x) == dict:
        return x.keys()

    return range(0, len(x))

def index_sets_agree(x, y):
    if type(x) == dict:
        keys1 = x.keys()
    else:
        keys1 = range(1, len(x)+1)

    if type(y) == dict:
        keys2 = y.keys()
    else:
        keys2 = range(1, len(y)+1)

    return keys1 == keys2

def arg_min(x):
    return min(x, key=x.get)

def mult(model, a, b):
    target = model.new_int_var(-4611686018427387, 4611686018427387, str(uuid.uuid4()))
    model.add_multiplication_equality(target, a, b)

    return target

def assert_(b, msg, x=None):
    assert b, msg
    return x

def symdiff_(a, b):
    return (a - b) | (b - a)

def dict_to_2d_array(d):
    first_elem = [ v[0] for v in d]

    return [
        [ d[k, j[1]] for j in d if j[0] == k]
        for k in first_elem
    ]

def ortools_all_different(model, v):
    if type(v) == dict:
        v = v.values()

    model.add_all_different(v)

def ortools_allowed_assignments(model, a, b):
    if type(a) == dict:
        a = a.values()
    if type(b) == dict:
        b = dict_to_2d_array(b)

    model.add_allowed_assignments(a, b)

def make_intervals(model, starts, sizes):
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

def ortools_cumulative(model, starts, sizes, demands, b):
    if type(starts) == dict:
        starts = starts.values()
    if type(sizes) == dict:
        sizes = sizes.values()
    if type(demands) == dict:
        demands = demands.values()

    intervals = make_intervals(model, starts, sizes)

    model.add_cumulative(intervals, demands, b)

def ortools_disjunctive_strict(model, starts, sizes):
    if type(starts) == dict:
        starts = starts.values()
    if type(sizes) == dict:
        sizes = sizes.values()


    model.add_no_overlap(make_intervals(model, starts, sizes))

def ortools_circuit(model, x, min_index, is_subcircuit=False):
    if type(x) == dict:
        x = x.values()

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

def ortools_count_eq(model, counts, var, target):
    vars = []
    for count in counts:
        vars.append(mors_lib_bool(model, var == count, var != value))

    model.add(sum(vars) == target)

def ortools_count_eq_cst(model, counts, value, target):
    ortools_count_eq(model, counts, value, target)

def ortools_diffn(model, x, y, dx, dy):
    x_intervals = make_intervals(model, x, dx)
    y_intervals = make_intervals(model, y, dy)

    model.add_no_overlap_2d(x_intervals, y_intervals)

def ortools_diffn_nonstrict(model, x, y, dx, dy):
    ortools_diffn(model, x, y, dx, dy)

# TODO
def ortools_network_flow_cost(model, arcs, num_nodes, base_node, flow, weight, cost):
    num_arcs = len(arcs) // 2

    for arc in range(num_arcs):
        tail = arcs[2 * arc] - base_node
        head = arcs[2 * arc + 1] - base_node

def ortools_network_flow(model, arcs, num_nodes, base_node, flow):
    ...
def ortools_regular(model, x, Q, S, d, q0, F):
    ...

def ortools_disjunctive(model, starts, sizes):
    if type(starts) == dict:
        starts = starts.values()
    if type(sizes) == dict:
        sizes = sizes.values()


    model.add_cumualtive(make_intervals(model, starts, sizes), [1 for _ in range(len(starts))], 1)


def ortools_inverse(model, f, invf, base_direct, base_inverse):
    if type(f) == dict:
        f = list(f.values())
    if type(invf) == dict:
        invf = list(invf.values())

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

def slice_dict(arr, indexes):
    def indexes_equal(ix1, ix2):
        if isinstance(ix1, Iterable):
            for i1, i2 in zip(ix1, ix2):
                if type(i2) == IntVar:
                    continue

                if i1 != i2:
                    return False
            return True
        else:
            if type(ix2) == IntVar:
                return True

            return ix1 != ix2
    
    return [
        v
        for k, v in arr.items()
        if indexes_equal(k, indexes)
    ]

def access(model, arr, indexes):
    def find_ix(ixs):
        if isinstance(ixs, Iterable):
            for i in ixs:
                if type(i) == IntVar:
                    return i
        else:
            return ixs

    def get_new_domain(arr):
        domain = Domain.FromValues([])
        for v in arr:
            if type(v) == IntVar:
                add_domain = Domain.FromFlatIntervals(v.proto.domain)
            else:
                add_domain = Domain.FromValues([v])
            domain = domain.union_with(add_domain)
        return domain

    def values_from_flat_interval(interval):
        values = []
        for i in range(0, len(interval), 2):
            values.extend(list(range(interval[i], interval[i+1]+1)))
        return values

            
    slice = slice_dict(arr, indexes)
    element = model.new_int_var_from_domain(get_new_domain(slice), str(uuid.uuid4()))

    ix_var = find_ix(indexes)
    new_ix = model.new_int_var(0, len(slice)-1, str(uuid.uuid4()))

    model.AddElement(new_ix, values_from_flat_interval(ix_var.proto.domain), ix_var)

    model.AddElement(new_ix, slice, element)
    return element

def int_min(model, a, b, m):
    min_bool = mors_lib_bool(model, model.Add(a < b), model.Add(a >= b))
    model.Add(m == a).only_enforce_if(min_bool)
    model.Add(m == b).only_enforce_if(~min_bool)
    return m

def mors_lib_bool(model, c1, c2):
    b = model.new_bool_var(str(uuid.uuid4()))
    c1.only_enforce_if(b)
    c2.only_enforce_if(~b)

    return b

def and_(model, a_bool, b_bool):
    c1 = model.add_bool_and(a_bool, b_bool)
    c2 = model.add_bool_or(~a_bool, ~b_bool)

    return mors_lib_bool(model, c1, c2)


def or_(model, a_bool, b_bool):
    c1 = model.add_bool_or(a_bool, b_bool)
    c2 = model.add_bool_and(~a_bool, ~b_bool)

    return mors_lib_bool(model, c1, c2)

def xor_(model, a_bool, b_bool):
    return ~equiv(a_bool, b_bool)

def forall_(model, bools):
    c1 = model.add_bool_and(bools)
    c2 = model.add_bool_or([~b for b in bools])

    return mors_lib_bool(model, c1, c2)

def impl_(model, a_bool, b_bool):
    c1 = model.add_implication(a_bool, b_bool)
    c2 = model.add_bool_and(a_bool, ~b_bool)

    return mors_lib_bool(model, c1, c2)

def not_(a):
    return a[1], a[0]

def abs_(model, a):
    target = model.new_int_var(-4611686018427387, 4611686018427387, str(uuid.uuid4()))

    model.add_abs_equality(target, a)
    return target

def mod_(model, a, b):
    target = model.new_int_var(-4611686018427387, 4611686018427387, str(uuid.uuid4()))

    model.add_modulo_equality(target, a, b)
    return target

def equiv_(model, a, b):
    return and_(model, impl_(model, a, b), impl_(model, b, a))

def in_(model, var, arr):
    bools = [model.new_bool_var(str(uuid.uuid4())) for _ in range(len(arr))]

    for value, b_ in zip(arr, bools):
        model.Add(var == value).only_enforce_if(b_)
        model.Add(var != value).only_enforce_if(~b_)

    return mors_lib_bool(model, model.add_bool_or(bools), model.add_bool_and([~b for b in bools]))

def bool2int(model, a_bool):
    b = model.new_int_var(0, 1, str(uuid.uuid4()))
    model.Add(b == 1).only_enforce_if(a_bool)
    model.Add(b == 0).only_enforce_if(~a_bool)

    return b

def show_int(a, b):
    return str(b).rjust(a) if a > 0 else str(b).ljust(a)
