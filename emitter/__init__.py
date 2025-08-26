from ir_python import *
import ast


class Emitter:
    def __init__(self):
        self.ast_tree = ast.Module()

    def init_file(self):    
        self.ast_tree.body.append(ast.parse("from ortools.sat.python import cp_model\nfrom itertools import product\n\n\nmodel=cp_model.CpModel()\n"))

    def finalize_file(self):
        self.ast_tree.body.append(ast.parse("solver = cp_model.CpSolver()\nstatus = solver.solve(model)\n"))
    #     TO BE DECIDED IN WHAT FORM WE FINALIZE MAIN 
    #     self.ast_tree.body.append( ast.parse("""
    #     if __name__ == "__main__":
    #         main()
    #     """)
    #     )

    def ast_const(self, decl: DeclConst):
        match(decl.type.type()):
            case "int" | "float" | "bool":
                print(f"{decl.id} = {self.ast_expr(decl.value, False)}")
                self.ast_tree.body.append(ast.parse(f"{decl.id} = {self.ast_expr(decl.value, False)}"))
            case "int_set" | "float_set" | "bool_set":
                self.ast_tree.body.append(ast.parse(f"{decl.id} = set({self.ast_expr(decl.value,False)})"))
            case "array":
                self.ast_tree.body.append(ast.parse(f"{decl.id} = dict({self.ast_LiteralArray(decl.value.get(), False, decl.type.dims)})"))
            case _:
                return ""
            
    def ast_expr(self, expr: ExprHandle, is_output:bool):
        if (type(expr.get())==BinOp):
            return self.ast_BinOp(expr.get(), is_output)
        elif (type(expr.get())==LiteralInt):
            return str(expr.get().value)
        elif (type(expr.get())==IdExpr):
            return str(expr.get().id) if not is_output else f"solver.Value({expr.get().id})"
        elif (type(expr.get())==LiteralString):
            return f"\"{expr.get().value.encode('unicode_escape').decode("utf-8")}\""
        elif (type(expr.get())==Call):
            return self.ast_Call(expr.get(), is_output)
        elif (type(expr.get())==Comprehension):
            return self.ast_Comprehension(expr.get(),is_output)
        elif(type(expr.get())==ArrayAccess):
            if len(expr.get().indexes)==1:
                return f"{self.ast_expr(expr.get().arr, is_output)}[{self.ast_expr(expr.get().indexes[0],is_output)}]"
            else:
                return f"{self.ast_expr(expr.get().arr, is_output)}[({",".join([self.ast_expr(ix,is_output) for ix in expr.get().indexes])})]"
        else :
            return ""

    def ast_BinOp(self, bin_op: BinOp, is_output: bool):
        match(bin_op.kind):
            case BinOp.OpKind.PLUS :
                return ""
            case BinOp.OpKind.MINUS :
                return ""
            case BinOp.OpKind.MULT :
                return ""
            case BinOp.OpKind.IDIV :
                return f"{self.ast_expr(bin_op.lhs, is_output)} // {self.ast_expr(bin_op.rhs, is_output)}"
            case BinOp.OpKind.DOTDOT:
                return f"range({self.ast_expr(bin_op.lhs, is_output)}, {self.ast_expr(bin_op.rhs, is_output)} +1)"
            case BinOp.OpKind.EQ :
                return ""
            case BinOp.OpKind.NQ:
                return f"{self.ast_expr(bin_op.lhs, is_output)} != {self.ast_expr(bin_op.rhs, is_output)}"
            case BinOp.OpKind.PLUSPLUS:
                return f"{self.ast_expr(bin_op.lhs, is_output)} + {self.ast_expr(bin_op.rhs, is_output)}"
            case _:
                return ""
            
    def ast_Call(self, call: Call, is_output: bool):
        match(call.id):
            case "\\29@format":
                return f"str({self.ast_expr(call.args[0], is_output)})"
            case "max":
                return f"max({self.ast_expr(call.args[0], is_output)})"
            case "min":
                return f"min({self.ast_expr(call.args[0], is_output)})"
            case _:
                return ""
    def ast_Comprehension(self, compr: Comprehension, is_output: bool):
        if is_output:
            ...
        else:
            return f"[{self.ast_expr(compr.body, is_output)}  {self.ast_generator(compr.generators, is_output)}]"
    def ast_generator(self, generators: list[Generator], is_output: bool):
        if is_output:
            ...
        else:
            gen=""
            for generator in generators:
                gen+= f"for {generator.variable.id} in {self.ast_expr(generator.in_expr, is_output)} "
            return gen
    def ast_var(self, decl: DeclVariable):
        match(decl.type.type()):
            case "int":
                self.ast_tree.body.append(ast.parse(f"{decl.id} = model.new_int_var_from_domain(cp_model.Domain.FromValues({self.ast_expr(decl.domain, False)}), \"{decl.id}\")"))
            case _:
                return ""
        #switch case which variable
    def ast_constraint(self, decl):
        self.ast_tree.body.append(ast.parse(f"model.Add({self.ast_expr(decl, False)})"))
        #not a clue so far
    def unparse_ast_tree(self, file_to_write):
        file_to_write.write(ast.unparse(self.ast_tree))
    def ast_output(self, out):
        if (type(out.get())==LiteralArray):
            self.ast_LiteralArray(out.get(), True, None)
        else:
            return ""
    def ast_LiteralArray(self, litarr: LiteralArray, is_output: bool, dimmensions: list[ExprHandle] | None):
        if is_output:
            for expr in litarr.value:
                self.ast_tree.body.append(ast.parse(f"print({self.ast_expr(expr, is_output)},end=\"\")"))
        else:
            literal_values="[" + "".join([self.ast_expr(expr, is_output) + "," for expr in litarr.value]) + "]"
            if len(dimmensions)==1:
                self.ast_tree.body.append(ast.parse(f"zip({self.ast_expr(dimmensions[0], is_output)},{literal_values})"))
            else:
                self.ast_tree.body.append(ast.parse(f"zip(product([{"".join([self.ast_expr(dimmensions[i], is_output)+ "," for i in range(len(dimmensions))])}],{literal_values}))"))

def hello_world(tree: Tree, file_path: str): # dir and filename to add
    print("hello from python")
    emitter = Emitter()
    file_to_write = open(file_path, "w")
    emitter.init_file()
    for decl in tree.decls:
        if type(decl) == DeclConst:
            print(decl.value)
            print(f"const {decl.id} {decl.type.type()} = {decl.value.get()}")
            emitter.ast_const(decl)

        # elif type(decl) == DeclVariable:
        #     print(f"var {decl.id} {decl.domain.get()} = {decl}")
        #     emitter.ast_var(decl)

    # for constraints in tree.constraints:
    #     emitter.ast_constraint(constraints)

    # emitter.finalize_file()
    # emitter.ast_output(tree.output)
    emitter.unparse_ast_tree(file_to_write)
    print(tree.solve_type)
    file_to_write.close()
