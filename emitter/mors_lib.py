import uuid
from ortools.sat.python.cp_model import Constraint

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

def show_int(a, b):
    return str(b).rjust(a) if a > 0 else str(b).ljust(a)
