from ir_python import Tree

def hello_world(tree: Tree):
    print("hello from python")
    for decl in tree.decls:
        print(decl.type(), decl.id)


