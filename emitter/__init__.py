from ir_python import Tree, DeclConst, DeclVariable

def hello_world(tree: Tree):
    print("hello from python")
    for decl in tree.decls:
        if type(decl) == DeclConst:
            print(f"const {decl.id} {decl.type.type()} = {decl.value.value}")
        elif type(decl) == DeclVariable:
            print("var")


