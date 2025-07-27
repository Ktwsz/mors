from ir_python import *

def hello_world(tree: Tree):
    print("hello from python")
    for decl in tree.decls:
        if type(decl) == DeclConst:
            print(decl.value)
            print(f"const {decl.id} {decl.type.type()} = {decl.value.get()}")
        elif type(decl) == DeclVariable:
            print("var")

    for constraints in tree.constraints:
        if constraints.get().kind == BinOp.OpKind.NQ:
            print("!=")

    print(tree.solve_type)

