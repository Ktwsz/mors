import uuid
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

def arrayXd(x, y):
    return dict(zip(x.keys(), y))

def arg_min(x):
    return min(x, key=x.get)

def mult(model, a, b):
    target = model.new_int_var(-4611686018427387, 4611686018427387, str(uuid.uuid4()))
    model.add_multiplication_equality(target, a, b)

    return target

def dict_to_2d_array(d):
    first_elem = [ v[0] for v in d]

    return [
        [ d[k, j[1]] for j in d if j[0] == k]
        for k in first_elem
    ]

def all_different(model, v):
    if type(v) == dict:
        v = v.values()

    model.add_all_different(v)

def allowed_assignments(model, a, b):
    if type(a) == dict:
        a = a.values()
    if type(b) == dict:
        b = dict_to_2d_array(b)

    model.add_allowed_assignments(a, b)

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

def finalize(a):
    a[0].only_enforce_if(True)
    a[1].only_enforce_if(False)

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
# ~(a -> b and b -> a) = (~(a -> b) or ~(b -> a)) = (~(~a or b) or ~(~b or a)) = ((a and ~b) or (b and ~a)) = ((a or b) and (~a or ~b))

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
