from ir_python import *
import ast


class Emitter:
    def __init__(self):
        self.ast_tree = ast.Module()

    def init_file(self):    
        self.ast_tree.body.append(ast.parse("from ortools.sat.python import cp_model\n\n\nmodel=cp_model.CpModel()\n"))

    def finalize_file(self):
        self.ast_tree.body.append(ast.parse("solver = cp_model.CpSolver()\nstatus = solver.solve(model)\n"))
    #     TO BE DECIDED IN WHAT FORM WE FINALIZE MAIN 
    #     self.ast_tree.body.append( ast.parse("""
    #     if __name__ == "__main__":
    #         main()
    #     """)
    #     )

    def ast_const(self, decl: DeclConst):
        self.ast_tree.body.append(ast.parse(f"{decl.id} = {decl.value.get().value}"))

    def ast_expr(self, expr: ExprHandle):
        if (type(expr.get())==BinOp):
            return self.ast_BinOp(expr.get())
        elif (type(expr.get())==LiteralInt):
            return str(expr.get().value)
        elif (type(expr.get())==IdExpr):
            return str(expr.get().id)
        elif (type(expr.get())==LiteralString):
            return f"\"{expr.get().value.encode('unicode_escape').decode("utf-8")}\""
        elif (type(expr.get())==Call):
            return self.ast_Call(expr.get())
        else :
            return ""

    def ast_BinOp(self, bin_op: BinOp):
        match(bin_op.kind):
            case BinOp.OpKind.PLUS :
                return ""
            case BinOp.OpKind.MINUS :
                return ""
            case BinOp.OpKind.MULT :
                return ""
            case BinOp.OpKind.IDIV :
                return ""
            case BinOp.OpKind.DOTDOT:
                return f"range({self.ast_expr(bin_op.lhs)}, {self.ast_expr(bin_op.rhs)} +1)"
            case BinOp.OpKind.EQ :
                return ""
            case BinOp.OpKind.NQ:
                return f"{self.ast_expr(bin_op.lhs)} != {self.ast_expr(bin_op.rhs)}"
            case BinOp.OpKind.PLUSPLUS:
                return f"{self.ast_expr(bin_op.lhs)} + {self.ast_expr(bin_op.rhs)}"
            case _:
                return ""
            
    def ast_Call(self, call: Call):
        match(call.id):
            case "\\29@format":
                return f"str({self.ast_expr(call.args[0])})"
            case _:
                return ""

    def ast_var(self, decl: DeclVariable):
        match(decl.type.type()):
            case "int":
                self.ast_tree.body.append(ast.parse(f"{decl.id} = model.new_int_var_from_domain(cp_model.Domain.FromValues({self.ast_expr(decl.domain)}), \"{decl.id}\")"))
            case _:
                return ""
        #switch case which variable
    def ast_constraint(self, decl):
        self.ast_tree.body.append(ast.parse(f"model.Add({self.ast_expr(decl)})"))
        #not a clue so far
    def unparse_ast_tree(self, file_to_write):
        file_to_write.write(ast.unparse(self.ast_tree))
    def ast_output(self, out):
        if (type(out.get())==LiteralArray):
            self.ast_LiteralArray(out.get())
        else:
            return ""
    def ast_LiteralArray(self, litarr: LiteralArray):
        for expr in litarr.value:
            print(f"print({self.ast_expr(expr)},end=\"\")")
            self.ast_tree.body.append(ast.parse(f"print({self.ast_expr(expr)},end=\"\")"))


def hello_world(tree: Tree): # dir and filename to add
    print("hello from python")
    emitter = Emitter()
    dir="."
    file_name="new_file.py"
    file_to_write = open(f"{dir}/{file_name}", "w")
    emitter.init_file()
    for decl in tree.decls:
        if type(decl) == DeclConst:
            print(decl.value)
            print(f"const {decl.id} {decl.type.type()} = {decl.value.get()}")
            emitter.ast_const(decl)

        elif type(decl) == DeclVariable:
            print(f"var {decl.id} {decl.domain.get()} = {decl}")
            emitter.ast_var(decl)

    for constraints in tree.constraints:
        emitter.ast_constraint(constraints)
    
    emitter.ast_output(tree.output)
    emitter.finalize_file()
    emitter.unparse_ast_tree(file_to_write)
    print(tree.solve_type)
    file_to_write.close()
