from ir_python import *
import ast

STATE_DEFAULT = 1
STATE_OUTPUT = 2
STATE_CONSTRAINT = 3

def is_output(state):
    return state == STATE_OUTPUT

def is_constraint(state):
    return state == STATE_CONSTRAINT

class Emitter:
    def __init__(self, tree):
        self.ast_tree = ast.Module()
        self.tree = tree
        self.to_generate = []

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
                self.ast_tree.body.append(ast.parse(f"{decl.id} = {self.ast_expr(decl.value, STATE_DEFAULT)}"))
            case "int_set" | "float_set" | "bool_set":
                self.ast_tree.body.append(ast.parse(f"{decl.id} = set({self.ast_expr(decl.value,STATE_DEFAULT)})"))
            case "array":
                self.ast_tree.body.append(ast.parse(f"{decl.id} = dict({self.ast_decl_LiteralArray(decl.value.get(), decl.type.dims)})"))
            case _:
                return ""
            
    def ast_expr(self, expr: ExprHandle, state):
        if (type(expr.get())==BinOp):
            if is_constraint(state):
                return f"model.Add({self.ast_BinOp(expr.get(), STATE_DEFAULT)})"

            return self.ast_BinOp(expr.get(), state)
        elif (type(expr.get())==LiteralInt):
            return str(expr.get().value)
        elif (type(expr.get())==LiteralBool):
            return str(expr.get().value)
        elif (type(expr.get())==LiteralArray):
            return self.ast_LiteralArray(expr.get(), state)
        elif (type(expr.get())==IdExpr):
            return str(expr.get().id) if not is_output(state) else f"solver.Value({expr.get().id})"
        elif (type(expr.get())==LiteralString):
            return f"\"{expr.get().value.encode('unicode_escape').decode("utf-8")}\""
        elif (type(expr.get())==Call):
            return self.ast_Call(expr.get(), state)
        elif (type(expr.get())==Comprehension):
            return self.ast_Comprehension(expr.get(),state)
        elif (type(expr.get())==IfThenElse):
            return self.ast_IfThenElse(expr.get(),state)
        elif(type(expr.get())==ArrayAccess):
            if len(expr.get().indexes)==1:
                return f"{self.ast_expr(expr.get().arr, state)}[{self.ast_expr(expr.get().indexes[0],state)}]"
            else:
                return f"{self.ast_expr(expr.get().arr, state)}[({", ".join([self.ast_expr(ix,state) for ix in expr.get().indexes])})]"
        else :
            return ""

    def ast_BinOp(self, bin_op: BinOp, state):
        match(bin_op.kind):
            case BinOp.OpKind.PLUS :
                return f"({self.ast_expr(bin_op.lhs, state)}) + ({self.ast_expr(bin_op.rhs, state)})"
            case BinOp.OpKind.MINUS :
                return f"({self.ast_expr(bin_op.lhs, state)}) - ({self.ast_expr(bin_op.rhs, state)})"
            case BinOp.OpKind.MULT :
                return f"({self.ast_expr(bin_op.lhs, state)}) * ({self.ast_expr(bin_op.rhs, state)})"
            case BinOp.OpKind.IDIV :
                return f"{self.ast_expr(bin_op.lhs, state)} // {self.ast_expr(bin_op.rhs, state)}"
            case BinOp.OpKind.DOTDOT:
                return f"range({self.ast_expr(bin_op.lhs, state)}, {self.ast_expr(bin_op.rhs, state)} +1)"
            case BinOp.OpKind.EQ :
                return f"{self.ast_expr(bin_op.lhs, state)} == {self.ast_expr(bin_op.rhs, state)}"
            case BinOp.OpKind.NQ:
                return f"{self.ast_expr(bin_op.lhs, state)} != {self.ast_expr(bin_op.rhs, state)}"
            case BinOp.OpKind.GQ:
                return f"{self.ast_expr(bin_op.lhs, state)} >= {self.ast_expr(bin_op.rhs, state)}"
            case BinOp.OpKind.GR:
                return f"{self.ast_expr(bin_op.lhs, state)} > {self.ast_expr(bin_op.rhs, state)}"
            case BinOp.OpKind.LE:
                return f"{self.ast_expr(bin_op.lhs, state)} < {self.ast_expr(bin_op.rhs, state)}"
            case BinOp.OpKind.LQ:
                return f"{self.ast_expr(bin_op.lhs, state)} <= {self.ast_expr(bin_op.rhs, state)}"
            case BinOp.OpKind.PLUSPLUS:
                return f"{self.ast_expr(bin_op.lhs, state)} + {self.ast_expr(bin_op.rhs, state)}"
            case BinOp.OpKind.AND:
                return f"{self.ast_expr(bin_op.lhs, state)} and {self.ast_expr(bin_op.rhs, state)}"
            case _:
                return ""
            
    def ast_Call(self, call: Call, state):
        match(call.id):
            case "format_29":
                return f"str({self.ast_expr(call.args[0], state)})"
            case "max":
                return f"max({self.ast_expr(call.args[0], STATE_DEFAULT)})" # TODO: match arg type
            case "min":
                return f"min({self.ast_expr(call.args[0], STATE_DEFAULT)})"
            case "sum":
                return f"sum({self.ast_expr(call.args[0], STATE_DEFAULT)})"
            case "ceil":
                return f"math.ceil({self.ast_expr(call.args[0], STATE_DEFAULT)})"
            case "log":
                return f"math.log({self.ast_expr(call.args[0], STATE_DEFAULT)})" # TODO: - 
            case "int2float":
                return f"float({self.ast_expr(call.args[0], STATE_DEFAULT)})" 
            case "array1d":
                return self.ast_expr(call.args[0], STATE_DEFAULT)
            case "forall":
                if is_constraint(state):
                    return self.ast_Comprehension(call.args[0].get(), state)
                else:
                    return f"all({self.ast_expr(call.args[0], STATE_DEFAULT)})"
            case "fzn_all_different_int":
                return f"model.add_all_different({self.ast_expr(call.args[0], STATE_DEFAULT)})"
            case _:
                if call.id not in self.to_generate:
                    self.to_generate.append(call.id)
                return f"{call.id}({", ".join([self.ast_expr(arg, STATE_DEFAULT) for arg in call.args])})"

    def ast_Function(self, function_id: str):
        if function_id not in self.tree.functions:
            return

        function = self.tree.functions[function_id]
        fn = ast.FunctionDef(function.id)
        fn.args = ast.arguments(args = [ast.arg(arg=fn_arg) for fn_arg in function.params])
        fn.body.append(ast.parse(self.ast_expr(function.body, STATE_DEFAULT)))
        self.ast_tree.body.append(ast.fix_missing_locations(fn))


    def ast_Comprehension(self, compr: Comprehension, state):
        if is_output(state):
            ...
        elif is_constraint(state):
            if_py, current = self.ast_generator(compr.generators, state)
            print(self.ast_expr(compr.body, state))
            current.body.append(ast.parse(self.ast_expr(compr.body, state)))
            return ast.unparse(ast.fix_missing_locations(if_py))
        else:
            return f"[{self.ast_expr(compr.body, state)}  {self.ast_generator(compr.generators, state)}]"

    def ast_IfThenElse(self, ite: IfThenElse, state):
        if is_constraint(state):
            if_py = ast.If(ast.parse(self.ast_expr(ite.if_then[0][0], STATE_DEFAULT)).body[0].value, ast.parse(self.ast_expr(ite.if_then[0][1], state)))
            current = if_py
            for cond, body in ite.if_then[1:]:
                current.orelse.append(ast.If(ast.parse(self.ast_expr(cond, STATE_DEFAULT)).body[0].value, ast.parse(self.ast_expr(body, state))))
            if ite.else_expr is not None:
                current.orelse.append(ast.parse(self.ast_expr(ite.else_expr, state)))
            return ast.unparse(ast.fix_missing_locations(if_py))
        else:
            ...


    def ast_generator(self, generators: list[Generator], state):
        if is_output(state):
            ...
        elif is_constraint(state):
            gen=ast.Module()
            current = gen
            for generator in generators:
                if type(generator) == Iterator:
                    current.body.append(ast.For(target=ast.Name(generator.variable.id), iter=ast.parse(self.ast_expr(generator.in_expr, STATE_DEFAULT)).body[0].value))
                else:
                    current.body.append(ast.If(ast.parse(self.ast_expr(generator, STATE_DEFAULT)).body[0].value))
                current = current.body[0]
            return gen, current
        else:
            gen=""
            for generator in generators:
                if type(generator) == Iterator:
                    gen+= f"for {generator.variable.id} in {self.ast_expr(generator.in_expr, state)} "
                else:
                    gen += f"if {self.ast_expr(generator, state)} "
            return gen

    def ast_declare_var(self, id: str, domain: ExprHandle):
        return f"model.new_int_var_from_domain(cp_model.Domain.FromValues({self.ast_expr(domain, STATE_DEFAULT)}), {id})"

    def ast_var(self, decl: DeclVariable):
        match(decl.type.type()):
            case "int":
                self.ast_tree.body.append(ast.parse(f"{decl.id} = {self.ast_declare_var(f"\"{decl.id}\"", decl.domain)}"))
            case "array":
                self.ast_tree.body.append(ast.parse(f"{decl.id} = {self.ast_var_Array(decl, STATE_DEFAULT)}"))
            case _:
                return ""
        #switch case which variable
    def ast_constraint(self, decl):
        print(self.ast_expr(decl, STATE_CONSTRAINT))
        self.ast_tree.body.append(ast.parse(self.ast_expr(decl, STATE_CONSTRAINT)))
        #not a clue so far
    def unparse_ast_tree(self, file_to_write):
        file_to_write.write(ast.unparse(self.ast_tree))

    def ast_output(self, out):
        if type(out.get())==LiteralArray:
            for print_expr in self.ast_output_LiteralArray(out.get()):
                self.ast_tree.body.append(ast.parse(print_expr))
        else:
            return ""

    def ast_var_Array(self, var: DeclVariable, state):
        dimensions = var.type.dims
        new_id = f"\"{var.id}\" + str(key)"
        
        if len(dimensions)==1:
            return f"{{key: {self.ast_declare_var(new_id, var.domain)} for key in {self.ast_expr(dimensions[0], state)} }}"
        else:
            return f"{{key: {self.ast_declare_var(new_id, var.domain)} for key in product({", ".join([self.ast_expr(dimensions[i], state) for i in range(len(dimensions))])}) }}"


    def ast_output_LiteralArray(self, litarr: LiteralArray):
        return [
            f"print({self.ast_expr(expr, STATE_OUTPUT)},end=\"\")"
            for expr in litarr.value
        ]

    def ast_LiteralArray(self, litarr: LiteralArray, state):
        return "[" + ", ".join([
            self.ast_expr(expr, state)
            for expr in litarr.value
        ]) + "]"

    def ast_decl_LiteralArray(self, litarr: LiteralArray, dimensions: list[ExprHandle] | None):
            literal_values="[" + ", ".join([self.ast_expr(expr, STATE_DEFAULT) for expr in litarr.value]) + "]"
            if len(dimensions)==1:
                return f"zip({self.ast_expr(dimensions[0], STATE_DEFAULT)},{literal_values})"
            else:
                return f"zip(product({", ".join([self.ast_expr(dimensions[i], STATE_DEFAULT) for i in range(len(dimensions))])}),{literal_values})"

def hello_world(tree: Tree, file_path: str):
    print("hello from python")
    emitter = Emitter(tree)
    file_to_write = open(file_path, "w")
    emitter.init_file()
    for decl in tree.decls:
        if type(decl) == DeclConst:
            emitter.ast_const(decl)

        elif type(decl) == DeclVariable:
            emitter.ast_var(decl)

    for constraints in tree.constraints:
        emitter.ast_constraint(constraints)

    i = 0
    while i < len(emitter.to_generate):
        emitter.ast_Function(emitter.to_generate[i])
        i += 1

    emitter.finalize_file()
    emitter.ast_output(tree.output)
    emitter.unparse_ast_tree(file_to_write)
    print(tree.solve_type)
    file_to_write.close()