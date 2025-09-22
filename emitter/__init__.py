from ir_python import *
import ast

SOLUTION_PRINTER_TEMPLATE = """
class VarArraySolutionPrinter(cp_model.CpSolverSolutionCallback):
    def __init__(self):
        cp_model.CpSolverSolutionCallback.__init__(self)
        self.__solution_count = 0

    def on_solution_callback(self) -> None:
        self.__solution_count += 1

    @property
    def solution_count(self) -> int:
        return self.__solution_count
"""

STATE_DEFAULT = 1
STATE_OUTPUT = 2
STATE_CONSTRAINT = 3
STATE_CONSTRAINT_EXPR = 4

def is_output(state):
    return state == STATE_OUTPUT

def is_constraint(state):
    return state == STATE_CONSTRAINT or state == STATE_CONSTRAINT_EXPR

def is_expr(state):
    return state == STATE_DEFAULT or state == STATE_CONSTRAINT_EXPR

def is_stmt(state):
    return state == STATE_OUTPUT or state == STATE_CONSTRAINT

class Emitter:
    def __init__(self, tree):
        self.ast_tree = ast.Module()
        self.tree = tree
        self.to_generate = []

    def init_file(self):    
        self.ast_tree.body.append(ast.parse("import math\nfrom ortools.sat.python import cp_model\nimport mors_lib\nfrom itertools import product\n\n\nmodel=cp_model.CpModel()\n"))

        template = ast.parse(SOLUTION_PRINTER_TEMPLATE).body[0]
        self.ast_tree.body.append(template)
        self.function_place = len(self.ast_tree.body)

        self.solution_printer_constructor = next(filter(lambda fn: fn.name == '__init__', template.body))
        self.solution_printer_callback = next(filter(lambda fn: fn.name == 'on_solution_callback', template.body))
                
        self.solution_printer_params = []

    def output_variable(self, id_expr):
        if not id_expr.is_global or id_expr.id in self.solution_printer_params:
            return
        
        name = id_expr.id
        self.solution_printer_params.append(name)
        self.solution_printer_constructor.args.args.append(ast.arg(arg=name))
        self.solution_printer_constructor.body.append(ast.parse(f"self.{name}={name}").body[0])


    def finalize_file(self):
        self.ast_tree.body.append(ast.parse("solver = cp_model.CpSolver()"))
        self.ast_tree.body.append(ast.parse(f"solution_printer = VarArraySolutionPrinter({", ".join(self.solution_printer_params)})"))
        self.ast_tree.body.append(ast.parse("status = solver.solve(model, solution_printer)\n"))
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
            return self.ast_BinOp(expr.get(), state)
        elif (type(expr.get())==LiteralInt):
            return str(expr.get().value)
        elif (type(expr.get())==LiteralBool):
            return str(expr.get().value)
        elif (type(expr.get())==LiteralArray):
            if is_output(state):
                return ast.unparse(ast.fix_missing_locations(ast.Module(body=[
                    ast.parse(f"print({self.ast_expr(expr, STATE_OUTPUT)},end=\"\")")
                    for expr in expr.get().value
                ])))

            return self.ast_LiteralArray(expr.get(), state)
        elif (type(expr.get())==IdExpr):
            if is_output(state):
                self.output_variable(expr.get())

                if expr.get().is_global:
                    name = f"self.{expr.get().id}"
                else:
                    name = expr.get().id

                if expr.get().is_var and expr.get().expr_type.type() != 'array':
                    return f"self.value({name})"
                else:
                    return name
            else:
                return str(expr.get().id)
        elif (type(expr.get())==LiteralString):
            return f"\"{expr.get().value.encode('unicode_escape').decode("utf-8")}\""
        elif (type(expr.get())==Call):
            return self.ast_Call(expr.get(), state)
        elif (type(expr.get())==Comprehension):
            return self.ast_Comprehension(expr.get(),state)
        elif (type(expr.get())==IfThenElse):
            return self.ast_IfThenElse(expr.get(),state)
        elif(type(expr.get())==ArrayAccess):
            # TODO - check for cosntraint expr state, if id is var and one of ixs is also var
            if is_output(state):
                return f"self.value({self.ast_expr(expr.get().arr, state)}[({", ".join([self.ast_expr(ix,state) for ix in expr.get().indexes])})])"
            else:
                print(expr.get().arr.get().id)
                print(is_constraint(state)  , expr.get().is_index_var_type)
                if is_constraint(state) and expr.get().is_index_var_type:
                    return f"mors_lib.access(model, {self.ast_expr(expr.get().arr, state)}, ({", ".join([self.ast_expr(ix,state) for ix in expr.get().indexes])}))"
                return f"{self.ast_expr(expr.get().arr, state)}[({", ".join([self.ast_expr(ix,state) for ix in expr.get().indexes])})]"
        else :
            return ""

    def ast_BinOp(self, bin_op: BinOp, state):
        lhs = self.ast_expr(bin_op.lhs, state)
        rhs = self.ast_expr(bin_op.rhs, state)

        match(bin_op.kind):
            case BinOp.OpKind.PLUS :
                return f"({lhs}) + ({rhs})"
            case BinOp.OpKind.MINUS :
                return f"({lhs}) - ({rhs})"
            case BinOp.OpKind.MULT :
                return f"({lhs}) * ({rhs})"
            case BinOp.OpKind.MOD :
                return f"({lhs}) % ({rhs})"
            case BinOp.OpKind.IDIV :
                return f"({lhs}) // ({rhs})"
            case BinOp.OpKind.DOTDOT:
                return f"range({lhs}, {rhs} +1)"
            case BinOp.OpKind.EQ :
                op = f"({lhs}) == ({rhs})"
                if is_constraint(state):
                    if is_stmt(state):
                        return f"model.Add({op})"

                    not_op = f"({lhs}) != ({rhs})"
                    return f"(model.Add({op}), model.Add({not_op}))"
                return op
            case BinOp.OpKind.NQ:
                op = f"({lhs}) != ({rhs})"
                if is_constraint(state):
                    if is_stmt(state):
                        return f"model.Add({op})"

                    not_op = f"({lhs}) == ({rhs})"
                    return f"(model.Add({op}), model.Add({not_op}))"
                return op
            case BinOp.OpKind.GQ:
                op = f"({lhs}) >= ({rhs})"
                if is_constraint(state):
                    if is_stmt(state):
                        return f"model.Add({op})"

                    not_op = f"({lhs}) < ({rhs})"
                    return f"(model.Add({op}), model.Add({not_op}))"
                return op
            case BinOp.OpKind.GR:
                op = f"({lhs}) > ({rhs})"
                if is_constraint(state):
                    if is_stmt(state):
                        return f"model.Add({op})"

                    not_op = f"({lhs}) <= ({rhs})"
                    return f"(model.Add({op}), model.Add({not_op}))"
                return op
            case BinOp.OpKind.LE:
                op = f"({lhs}) < ({rhs})"
                if is_constraint(state):
                    if is_stmt(state):
                        return f"model.Add({op})"

                    not_op = f"({lhs}) >= ({rhs})"
                    return f"(model.Add({op}), model.Add({not_op}))"
                return op
            case BinOp.OpKind.LQ:
                op = f"({lhs}) <= ({rhs})"
                if is_constraint(state):
                    if is_stmt(state):
                        return f"model.Add({op})"

                    not_op = f"({lhs}) > ({rhs})"
                    return f"(model.Add({op}), model.Add({not_op}))"
                return op
            case BinOp.OpKind.PLUSPLUS:
                if is_output(state) and bin_op.lhs.get().expr_type.type() == "array":
                    return f"{self.ast_expr(bin_op.lhs, state)}\n{self.ast_expr(bin_op.rhs, state)}"
                return f"({self.ast_expr(bin_op.lhs, state)}) + ({self.ast_expr(bin_op.rhs, state)})"
            case BinOp.OpKind.AND:
                if is_constraint(state):
                    if is_stmt(state):
                        return f"mors_lib.finalize(mors_lib.and_(model, {self.ast_expr(bin_op.lhs, STATE_CONSTRAINT_EXPR)}, {self.ast_expr(bin_op.rhs, STATE_CONSTRAINT_EXPR)}))"
                    return f"mors_lib.and_(model, {self.ast_expr(bin_op.lhs, STATE_CONSTRAINT_EXPR)}, {self.ast_expr(bin_op.rhs, STATE_CONSTRAINT_EXPR)})"

                return f"({lhs}) and ({rhs})"
            case BinOp.OpKind.OR:
                if is_constraint(state):
                    if is_stmt(state):
                        return f"mors_lib.finalize(mors_lib.or_(model, {self.ast_expr(bin_op.lhs, STATE_CONSTRAINT_EXPR)}, {self.ast_expr(bin_op.rhs, STATE_CONSTRAINT_EXPR)}))"
                    return f"mors_lib.or_(model, {self.ast_expr(bin_op.lhs, STATE_CONSTRAINT_EXPR)}, {self.ast_expr(bin_op.rhs, STATE_CONSTRAINT_EXPR)})"

                return f"({lhs}) or ({rhs})"
            case BinOp.OpKind.IMPL:
                if is_constraint(state):
                    if is_stmt(state):
                        return f"mors_lib.finalize(mors_lib.impl_(model, {self.ast_expr(bin_op.lhs, STATE_CONSTRAINT_EXPR)}, {self.ast_expr(bin_op.rhs, STATE_CONSTRAINT_EXPR)}))"
                    return f"mors_lib.impl_(model, {self.ast_expr(bin_op.lhs, STATE_CONSTRAINT_EXPR)}, {self.ast_expr(bin_op.rhs, STATE_CONSTRAINT_EXPR)})"

                return f"" #TODO
            case _:
                return ""
            
    def ast_Call(self, call: Call, state):
        match(call.id):
            case "format_29" | "format_31" | "format_32" | "format_33" | "format_40" | "show" :
                return f"str({self.ast_expr(call.args[0], state)})"
            case "max" | "card":
                arg = self.ast_expr(call.args[0], STATE_DEFAULT)
                if type(call.args[0].get()) == IdExpr and call.args[0].get().expr_type.type() == 'array':
                    arg += '.values()'
                return f"max({arg})"
            case "enum_next":
                return f"{self.ast_expr(call.args[1], STATE_DEFAULT)} + 1"
            case "min":
                return f"min({self.ast_expr(call.args[0], STATE_DEFAULT)})"
            case "sum":
                return f"sum({self.ast_expr(call.args[0], STATE_DEFAULT)})"
            case "ceil":
                return f"math.ceil({self.ast_expr(call.args[0], STATE_DEFAULT)})"
            case "log":
                return f"math.log({self.ast_expr(call.args[1], STATE_DEFAULT)})" 
            case "int2float":
                return f"float({self.ast_expr(call.args[0], STATE_DEFAULT)})" 
            case "array1d":
                return self.ast_expr(call.args[0], STATE_DEFAULT)
            case "forall":
                if is_constraint(state):
                    if is_stmt(state):
                        return self.ast_Comprehension(call.args[0].get(), state)

                    return f"mors_lib.forall_(model, {self.ast_expr(call.args[0], STATE_CONSTRAINT_EXPR)})"

                return f"all({self.ast_expr(call.args[0], STATE_DEFAULT)})"
            case "fzn_all_different_int":
                return f"model.add_all_different({self.ast_expr(call.args[0], STATE_DEFAULT)})"
            case "show_int":
                return f"mors_lib.show_int({self.ast_expr(call.args[0],state)}, {self.ast_expr(call.args[1],state)})"
            case _:
                if (call.id, state) not in self.to_generate:
                    self.to_generate.append((call.id, state))
                return f"{call.id}({", ".join([self.ast_expr(arg, STATE_DEFAULT) for arg in call.args])})"

    def ast_Function(self, function_id: str, state):
        if function_id not in self.tree.functions:
            return

        function = self.tree.functions[function_id]
        fn = ast.FunctionDef(function.id)
        fn.args = ast.arguments(args = [ast.arg(arg=fn_arg) for fn_arg in function.params])

        if is_output(state):
            fn.body.append(ast.parse(self.ast_expr(function.body, state)))
        else:
            fn.body.append(ast.Return(ast.parse(self.ast_expr(function.body, STATE_DEFAULT)).body[0].value))

        self.ast_tree.body.insert(self.function_place, ast.fix_missing_locations(fn))


    def ast_Comprehension(self, compr: Comprehension, state):
        if is_stmt(state):
            if_py, current = self.ast_generator(compr.generators, state)
            print(self.ast_expr(compr.body, state))
            if is_output(state):
                current.body.append(ast.parse(f"print({self.ast_expr(compr.body, state)}, end=\"\")"))
            else:
                current.body.append(ast.parse(self.ast_expr(compr.body, state)))
            return ast.unparse(ast.fix_missing_locations(if_py))
        else:
            return f"[{self.ast_expr(compr.body, state)}  {self.ast_generator(compr.generators, STATE_DEFAULT)}]"

    def ast_IfThenElse(self, ite: IfThenElse, state):
        if is_constraint(state) and is_stmt(state):
            if_py = ast.If(ast.parse(self.ast_expr(ite.if_then[0][0], STATE_DEFAULT)).body[0].value, ast.parse(self.ast_expr(ite.if_then[0][1], state)))
            current = if_py
            for cond, body in ite.if_then[1:]:
                current.orelse.append(ast.If(ast.parse(self.ast_expr(cond, STATE_DEFAULT)).body[0].value, ast.parse(self.ast_expr(body, state))))
            if ite.else_expr is not None:
                current.orelse.append(ast.parse(self.ast_expr(ite.else_expr, state)))
            return ast.unparse(ast.fix_missing_locations(if_py))
        else:
            current_orelse = ast.parse(self.ast_expr(ite.else_expr, state)).body[0].value
            for cond, body in ite.if_then[::-1]:
                current_cond = ast.parse(self.ast_expr(cond, STATE_DEFAULT)).body[0].value
                current_body = ast.parse(self.ast_expr(body, state)).body[0].value
                current_orelse = ast.IfExp(current_cond, current_body, current_orelse)

            return ast.unparse(ast.fix_missing_locations(current_orelse))


    def ast_generator(self, generators: list[Generator], state):
        if is_stmt(state):
            gen=ast.Module()
            current = gen
            for generator in generators:
                if type(generator) == Iterator:
                    current.body.append(ast.For(target=ast.Name(generator.variable.id), iter=ast.parse(self.ast_expr(generator.in_expr, state if is_output(state) else STATE_DEFAULT)).body[0].value))
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
        if domain is None:
            return f"model.new_int_var_from_domain(cp_model.Domain.FromValues(range(-2147483648, 2147483647)), {id})"

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
        self.solution_printer_callback.body.append(ast.parse(self.ast_expr(out, STATE_OUTPUT)))

    def ast_var_Array(self, var: DeclVariable, state):
        dimensions = var.type.dims
        new_id = f"\"{var.id}\" + str(key)"
        
        if len(dimensions)==1:
            return f"{{key: {self.ast_declare_var(new_id, var.domain)} for key in {self.ast_expr(dimensions[0], state)} }}"
        else:
            return f"{{key: {self.ast_declare_var(new_id, var.domain)} for key in product({", ".join([self.ast_expr(dimensions[i], state) for i in range(len(dimensions))])}) }}"


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

    def ast_model_solve_type(self, solve_type):
        if (type(solve_type)==SolveTypeMax):
            self.ast_tree.body.append(ast.parse(f"model.maximize({self.ast_expr(solve_type.expr, STATE_DEFAULT)})"))
        elif(type(solve_type)==SolveTypeMin):
            self.ast_tree.body.append(ast.parse(f"model.minimize({self.ast_expr(solve_type.expr, STATE_DEFAULT)})"))
        else:
            ...

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
        emitter.ast_Function(emitter.to_generate[i][0], emitter.to_generate[i][1])
        i += 1
    
    emitter.ast_model_solve_type(tree.solve_type)
    emitter.ast_output(tree.output)
    emitter.finalize_file()
    emitter.unparse_ast_tree(file_to_write)
    print(tree.solve_type)
    file_to_write.close()
