import uuid
from collections.abc import Iterable
from ortools.sat.python.cp_model import IntVar, Domain

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
            

    slice = slice_dict(arr, indexes)
    element = model.new_int_var_from_domain(Domain.FromValues(range(1, 6)), str(uuid.uuid4())) # copy domain
    model.AddElement(find_ix(indexes), slice, element)
    return element

def finalize(a):
    a[0].only_enforce_if(True)
    a[1].only_enforce_if(False)

def and_(model, a, b):
    a, not_a = a

    a_bool = model.new_bool_var(str(uuid.uuid4()))
    a.only_enforce_if(a_bool)
    not_a.only_enforce_if(~a_bool)

    b, not_b = b

    b_bool = model.new_bool_var(str(uuid.uuid4()))
    b.only_enforce_if(b_bool)
    not_b.only_enforce_if(~b_bool)

    return model.add_bool_and(a_bool, b_bool), model.add_bool_or(~a_bool, ~b_bool)


def or_(model, a, b):
    a, not_a = a

    a_bool = model.new_bool_var(str(uuid.uuid4()))
    a.only_enforce_if(a_bool)
    not_a.only_enforce_if(~a_bool)

    b, not_b = b

    b_bool = model.new_bool_var(str(uuid.uuid4()))
    b.only_enforce_if(b_bool)
    not_b.only_enforce_if(~b_bool)

    return model.add_bool_or(a_bool, b_bool), model.add_bool_and(~a_bool, ~b_bool)

def forall_(model, comp):
    bools = [model.new_bool_var(str(uuid.uuid4())) for _ in range(len(comp))]

    for (con, not_con), b in zip(comp, bools):
        con.only_enforce_if(b)
        not_con.only_enforce_if(~b)

    return model.add_bool_and(bools), model.add_bool_or([~b for b in bools])

def impl_(model, a, b):
    a, not_a = a

    a_bool = model.new_bool_var(str(uuid.uuid4()))
    a.only_enforce_if(a_bool)
    not_a.only_enforce_if(~a_bool)

    b, not_b = b

    b_bool = model.new_bool_var(str(uuid.uuid4()))
    b.only_enforce_if(b_bool)
    not_b.only_enforce_if(~b_bool)

    return model.add_implication(a_bool, b_bool), model.add_bool_and(a_bool, ~b_bool)

def show_int(a, b):
    return str(b).rjust(a) if a > 0 else str(b).ljust(a)
