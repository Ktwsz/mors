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

def is_id_expr_array(expr: ExprHandle):
    return type(expr.get()) == IdExpr and expr.get().expr_type.type() == 'array' 

class Emitter:
    def __init__(self, tree):
        self.ast_tree = ast.Module()
        self.tree = tree
        self.to_generate = []
        self.let_in_scope = [[]]
        self.let_map = {}

    def init_file(self):    
        self.ast_tree.body.append(ast.parse("import math\nfrom ortools.sat.python import cp_model\nfrom mors_lib import *\nfrom itertools import product\n\n\nmodel=cp_model.CpModel()\n"))

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

    def ast_const(self, decl: DeclConst):
        match(decl.type.type()):
            case "int" | "float" | "bool":
                if decl.id == "def":
                    id = "def_"
                else:
                    id = decl.id
                return ast.parse(f"{id} = {self.ast_expr(decl.value, STATE_DEFAULT)}")
            case "int_set" | "float_set" | "bool_set":
                return ast.parse(f"{decl.id} = set({self.ast_expr(decl.value,STATE_DEFAULT)})")
            case "array":
                expr = self.ast_expr(decl.value, STATE_DEFAULT)
                if type(decl.value.get()) == LiteralArray:
                    dimensions = decl.type.dims
                    if len(dimensions) == 1:
                        expr = f"zip({self.ast_expr(dimensions[0], STATE_DEFAULT)},{expr})"
                    else:
                        expr = f"zip(product({", ".join([self.ast_expr(dimensions[i], STATE_DEFAULT) for i in range(len(dimensions))])}),{expr})"

                    expr =f"dict({expr})"
                return ast.parse(f"{decl.id} = {expr}")
            case _:
                return ""

    def ast_expr(self, expr: ExprHandle, state):
        if (type(expr.get())==BinOp):
            return self.ast_BinOp(expr.get(), state)
        elif (type(expr.get())==UnaryOp):
            return self.ast_UnaryOp(expr.get(), state)
        elif (type(expr.get())==LiteralInt):
            return str(expr.get().value)
        elif (type(expr.get())==LiteralBool):
            if is_constraint(state):
                if is_stmt(state):
                    return f"model.Add({expr.get().value})"

                return f"mors_lib_bool(model, model.Add({expr.get().value}), model.Add({not expr.get().value}))"

            return str(expr.get().value)
        elif (type(expr.get())==LiteralArray):
            if is_output(state):
                return ast.unparse(ast.fix_missing_locations(ast.Module(body=[
                    ast.parse(f"print({self.ast_expr(expr, STATE_OUTPUT)},end=\"\")")
                    for expr in expr.get().value
                ])))

            return self.ast_LiteralArray(expr.get(), state)
        elif (type(expr.get())==LiteralSet):
            return self.ast_LiteralSet(expr.get(), state)

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
                if expr.get().id == "def":
                    return "def_"
                return str(expr.get().id)
        elif (type(expr.get())==LiteralString):
            return f"\"{expr.get().value.encode('unicode_escape').decode("utf-8")}\""
        elif (type(expr.get())==Call):
            return self.ast_Call(expr.get(), state)
        elif (type(expr.get())==Comprehension):
            return self.ast_Comprehension(expr.get(),state)
        elif (type(expr.get())==IfThenElse):
            return self.ast_IfThenElse(expr.get(),state)
        elif (type(expr.get())==LetIn):
            return self.ast_LetIn(expr.get(), state)
        elif(type(expr.get())==ArrayAccess):
            if is_output(state):
                return f"self.value({self.ast_expr(expr.get().arr, state)}[({", ".join([self.ast_expr(ix,state) for ix in expr.get().indexes])})])"
            else:
                if expr.get().is_index_var_type:
                    return f"access(model, {self.ast_expr(expr.get().arr, state)}, ({", ".join([self.ast_expr(ix,state) for ix in expr.get().indexes])}))"
                return f"{self.ast_expr(expr.get().arr, state)}[({", ".join([self.ast_expr(ix,state) for ix in expr.get().indexes])})]"
        else :
            return ""

    def ast_BinOp(self, bin_op: BinOp, state):

        match(bin_op.kind):
            case BinOp.OpKind.PLUS :
                lhs = self.ast_expr(bin_op.lhs, state)
                rhs = self.ast_expr(bin_op.rhs, state)

                return f"({lhs}) + ({rhs})"
            case BinOp.OpKind.MINUS :
                lhs = self.ast_expr(bin_op.lhs, state)
                rhs = self.ast_expr(bin_op.rhs, state)

                return f"({lhs}) - ({rhs})"
            case BinOp.OpKind.MULT :
                lhs = self.ast_expr(bin_op.lhs, state)
                rhs = self.ast_expr(bin_op.rhs, state)

                if bin_op.lhs.get().is_var and bin_op.rhs.get().is_var:
                    return f"mult(model, {lhs}, {rhs})"

                return f"({lhs}) * ({rhs})"
            case BinOp.OpKind.MOD :
                lhs = self.ast_expr(bin_op.lhs, state)
                rhs = self.ast_expr(bin_op.rhs, state)

                if is_constraint(state):
                    return f"mod_(model, {lhs}, {rhs})"

                return f"({lhs}) % ({rhs})"
            case BinOp.OpKind.IDIV :
                lhs = self.ast_expr(bin_op.lhs, state)
                rhs = self.ast_expr(bin_op.rhs, state)

                return f"({lhs}) // ({rhs})"
            case BinOp.OpKind.DIV :
                lhs = self.ast_expr(bin_op.lhs, state)
                rhs = self.ast_expr(bin_op.rhs, state)

                return f"({lhs}) / ({rhs})"
            case BinOp.OpKind.POW :
                lhs = self.ast_expr(bin_op.lhs, state)
                rhs = self.ast_expr(bin_op.rhs, state)

                return f"({lhs}) ** ({rhs})"
            case BinOp.OpKind.DOTDOT:
                lhs = self.ast_expr(bin_op.lhs, state)
                rhs = self.ast_expr(bin_op.rhs, state)

                return f"range({lhs}, {rhs} +1)"
            case BinOp.OpKind.EQ :
                if is_constraint(state):
                    if is_stmt(state):
                        return f"model.Add(({self.ast_expr(bin_op.lhs, STATE_CONSTRAINT_EXPR)}) == ({self.ast_expr(bin_op.rhs, STATE_CONSTRAINT_EXPR)}))"

                    lhs = self.ast_expr(bin_op.lhs, state)
                    rhs = self.ast_expr(bin_op.rhs, state)
                    op = f"({lhs}) == ({rhs})"

                    if bin_op.lhs.get().expr_type.type() == "int":
                        not_op = f"({lhs}) != ({rhs})"
                        return f"mors_lib_bool(model, model.Add({op}), model.Add({not_op}))"
                    else:
                        return f"model.Add({op})"

                return f"({self.ast_expr(bin_op.lhs, state)}) == ({self.ast_expr(bin_op.rhs, state)})"
            case BinOp.OpKind.NQ:
                lhs = self.ast_expr(bin_op.lhs, state)
                rhs = self.ast_expr(bin_op.rhs, state)

                op = f"({lhs}) != ({rhs})"
                if is_constraint(state):
                    if is_stmt(state):
                        return f"model.Add({op})"

                    if bin_op.lhs.get().expr_type.type() == "int":
                        not_op = f"({lhs}) == ({rhs})"
                        return f"mors_lib_bool(model, model.Add({op}), model.Add({not_op}))"
                    else:
                        return f"model.Add({op})"
                return op
            case BinOp.OpKind.GQ:
                lhs = self.ast_expr(bin_op.lhs, state)
                rhs = self.ast_expr(bin_op.rhs, state)

                op = f"({lhs}) >= ({rhs})"
                if is_constraint(state):
                    if is_stmt(state):
                        return f"model.Add({op})"

                    if bin_op.lhs.get().expr_type.type() == "int":
                        not_op = f"({lhs}) < ({rhs})"
                        return f"mors_lib_bool(model, model.Add({op}), model.Add({not_op}))"
                    else:
                        return f"model.Add({op})"
                return op
            case BinOp.OpKind.GR:
                lhs = self.ast_expr(bin_op.lhs, state)
                rhs = self.ast_expr(bin_op.rhs, state)

                op = f"({lhs}) > ({rhs})"
                if is_constraint(state):
                    if is_stmt(state):
                        return f"model.Add({op})"

                    if bin_op.lhs.get().expr_type.type() == "int":
                        not_op = f"({lhs}) <= ({rhs})"
                        return f"mors_lib_bool(model, model.Add({op}), model.Add({not_op}))"
                    else:
                        return f"model.Add({op})"
                return op
            case BinOp.OpKind.LE:
                lhs = self.ast_expr(bin_op.lhs, state)
                rhs = self.ast_expr(bin_op.rhs, state)

                op = f"({lhs}) < ({rhs})"
                if is_constraint(state):
                    if is_stmt(state):
                        return f"model.Add({op})"

                    if bin_op.lhs.get().expr_type.type() == "int":
                        not_op = f"({lhs}) >= ({rhs})"
                        return f"mors_lib_bool(model, model.Add({op}), model.Add({not_op}))"
                    else:
                        return f"model.Add({op})"
                return op
            case BinOp.OpKind.LQ:
                lhs = self.ast_expr(bin_op.lhs, state)
                rhs = self.ast_expr(bin_op.rhs, state)

                op = f"({lhs}) <= ({rhs})"
                if is_constraint(state):
                    if is_stmt(state):
                        return f"model.Add({op})"

                    if bin_op.lhs.get().expr_type.type() == "int":
                        not_op = f"({lhs}) > ({rhs})"
                        return f"mors_lib_bool(model, model.Add({op}), model.Add({not_op}))"
                    else:
                        return f"model.Add({op})"
                return op
            case BinOp.OpKind.PLUSPLUS:
                lhs = self.ast_expr(bin_op.lhs, state)
                rhs = self.ast_expr(bin_op.rhs, state)

                if is_output(state) and bin_op.lhs.get().expr_type.type() == "array":
                    return f"{lhs}\n{rhs}"
                return f"({lhs}) + ({rhs})"
            case BinOp.OpKind.AND:
                if is_constraint(state):
                    if is_stmt(state):
                        return f"{self.ast_expr(bin_op.lhs, state)}\n{self.ast_expr(bin_op.rhs, state)}"
                    return f"and_(model, {self.ast_expr(bin_op.lhs, STATE_CONSTRAINT_EXPR)}, {self.ast_expr(bin_op.rhs, STATE_CONSTRAINT_EXPR)})"

                return f"({self.ast_expr(bin_op.lhs, state)}) and ({self.ast_expr(bin_op.rhs, state)})"
            case BinOp.OpKind.OR:
                if is_constraint(state):
                    if is_stmt(state):
                        return f"model.Add(or_(model, {self.ast_expr(bin_op.lhs, STATE_CONSTRAINT_EXPR)}, {self.ast_expr(bin_op.rhs, STATE_CONSTRAINT_EXPR)}) == True)"
                    return f"or_(model, {self.ast_expr(bin_op.lhs, STATE_CONSTRAINT_EXPR)}, {self.ast_expr(bin_op.rhs, STATE_CONSTRAINT_EXPR)})"

                return f"({self.ast_expr(bin_op.lhs, state)}) or ({self.ast_expr(bin_op.rhs, state)})"
            case BinOp.OpKind.XOR:
                if is_constraint(state):
                    if is_stmt(state):
                        return f"model.Add(xor_(model, {self.ast_expr(bin_op.lhs, STATE_CONSTRAINT_EXPR)}, {self.ast_expr(bin_op.rhs, STATE_CONSTRAINT_EXPR)}) == True)"
                    return f"xor_(model, {self.ast_expr(bin_op.lhs, STATE_CONSTRAINT_EXPR)}, {self.ast_expr(bin_op.rhs, STATE_CONSTRAINT_EXPR)})"

                return f"bool({self.ast_expr(bin_op.lhs, state)}) != bool({self.ast_expr(bin_op.rhs, state)})"
            case BinOp.OpKind.IMPL:
                if is_constraint(state):
                    if is_stmt(state):
                        return f"model.Add(impl_(model, {self.ast_expr(bin_op.lhs, STATE_CONSTRAINT_EXPR)}, {self.ast_expr(bin_op.rhs, STATE_CONSTRAINT_EXPR)}) == True)"
                    return f"impl_(model, {self.ast_expr(bin_op.lhs, STATE_CONSTRAINT_EXPR)}, {self.ast_expr(bin_op.rhs, STATE_CONSTRAINT_EXPR)})"

                return f"" #TODO
            case BinOp.OpKind.RIMPL:
                if is_constraint(state):
                    if is_stmt(state):
                        return f"model.Add(impl_(model, {self.ast_expr(bin_op.rhs, STATE_CONSTRAINT_EXPR)}, {self.ast_expr(bin_op.lhs, STATE_CONSTRAINT_EXPR)}) == True)"
                    return f"impl_(model, {self.ast_expr(bin_op.rhs, STATE_CONSTRAINT_EXPR)}, {self.ast_expr(bin_op.lhs, STATE_CONSTRAINT_EXPR)})"

                return f"" #TODO
            case BinOp.OpKind.EQUIV:
                if is_constraint(state):
                    if is_stmt(state):
                        return f"model.Add(equiv_(model, {self.ast_expr(bin_op.lhs, STATE_CONSTRAINT_EXPR)}, {self.ast_expr(bin_op.rhs, STATE_CONSTRAINT_EXPR)}) == True)"
                    return f"equiv_(model, {self.ast_expr(bin_op.lhs, STATE_CONSTRAINT_EXPR)}, {self.ast_expr(bin_op.rhs, STATE_CONSTRAINT_EXPR)})"

                return f"({self.ast_expr(bin_op.lhs, state)}) == ({self.ast_expr(bin_op.rhs, state)})"
            case BinOp.OpKind.IN:
                if is_constraint(state):
                    if is_stmt(state):
                        return f"model.Add(in_(model, {self.ast_expr(bin_op.lhs, STATE_CONSTRAINT_EXPR)}, {self.ast_expr(bin_op.rhs, STATE_CONSTRAINT_EXPR)}) == True)"
                    return f"in_(model, {self.ast_expr(bin_op.lhs, STATE_CONSTRAINT_EXPR)}, {self.ast_expr(bin_op.rhs, STATE_CONSTRAINT_EXPR)})"

                return f"({self.ast_expr(bin_op.lhs, state)}) in ({self.ast_expr(bin_op.rhs, state)})"
            case BinOp.OpKind.DIFF:
                lhs = self.ast_expr(bin_op.lhs, state)
                rhs = self.ast_expr(bin_op.rhs, state)

                return f"({lhs}) - ({rhs})"
            case BinOp.OpKind.INTERSECT:
                lhs = self.ast_expr(bin_op.lhs, state)
                rhs = self.ast_expr(bin_op.rhs, state)

                return f"({lhs}) & ({rhs})"
            case BinOp.OpKind.UNION:
                lhs = self.ast_expr(bin_op.lhs, state)
                rhs = self.ast_expr(bin_op.rhs, state)

                return f"({lhs}) | ({rhs})"
            case BinOp.OpKind.SYMDIFF:
                lhs = self.ast_expr(bin_op.lhs, state)
                rhs = self.ast_expr(bin_op.rhs, state)

                return f"symdiff_({lhs}), ({rhs})"
            case BinOp.OpKind.SUBSET:
                lhs = self.ast_expr(bin_op.lhs, state)
                rhs = self.ast_expr(bin_op.rhs, state)

                return f"({lhs}) <= ({rhs})"
            case BinOp.OpKind.SUPERSET:
                lhs = self.ast_expr(bin_op.lhs, state)
                rhs = self.ast_expr(bin_op.rhs, state)

                return f"({lhs}) >= ({rhs})"
            case _:
                return ""

    def ast_UnaryOp(self, unary_op: UnaryOp, state):
        match(unary_op.kind):
            case UnaryOp.OpKind.NOT:
                if is_constraint(state):
                    if is_expr(state):
                        return f"~({self.ast_expr(unary_op.expr, state)})"

                    return f"model.Add(~({self.ast_expr(unary_op.expr, STATE_CONSTRAINT_EXPR)}) == True)"

                return f"not ({self.ast_expr(unary_op.expr, state)})"
            case UnaryOp.OpKind.PLUS:
                return f"+({self.ast_expr(unary_op.expr, state)}"
            case UnaryOp.OpKind.MINUS:
                return f"-({self.ast_expr(unary_op.expr, state)}"


    def ast_Call(self, call: Call, state):
        if is_constraint(state) and is_expr(state) and call.id + "_reif" in self.tree.functions:
            if (call.id + "_reif", state) not in self.to_generate:
                self.to_generate.append((call.id + "_reif", state))
            return f"{call.id + "_reif"}(model, {", ".join([self.ast_expr(arg, STATE_DEFAULT) for arg in call.args])})"

        match(call.id):
            case "format_29" | "format_30" | "format_31" | "format_32" | "format_33" | "format_40" | "format_41" | "format_42" | "show" :
                if is_id_expr_array(call.args[0]): 
                    if call.args[0].get().is_var:
                        return f"str([ self.value(v) for v in {self.ast_expr(call.args[0], state)}.values()])"
                    else:
                        return f"str([ self.value(v) for v in {self.ast_expr(call.args[0], state)}])"
                return f"str({self.ast_expr(call.args[0], state)})"
            case "card":
                arg = self.ast_expr(call.args[0], STATE_DEFAULT)
                if is_id_expr_array(call.args[0]): 
                    arg += '.values()'
                return f"max({arg})"
            case "enum_next":
                return f"{self.ast_expr(call.args[1], STATE_DEFAULT)} + 1"
            case "to_enum" | "to_enum_31":
                return f"{self.ast_expr(call.args[1], state)}"
            case "arg_min":
                return f"arg_min({self.ast_expr(call.args[0], STATE_DEFAULT)})"
            case "min" | "max":
                args = []
                for arg in call.args:
                    args.append(self.ast_expr(arg, STATE_DEFAULT))
                    if is_id_expr_array(arg): 
                        args[-1] += '.values()'
                return f"{call.id}({", ".join(args)})"
            case "int_min":
                return f"int_min(model, {", ".join([self.ast_expr(arg, STATE_DEFAULT) for arg in call.args])})"
            case "sum":
                return f"sum({self.ast_expr(call.args[0], STATE_DEFAULT)})"
            case "ceil":
                return f"math.ceil({self.ast_expr(call.args[0], STATE_DEFAULT)})"
            case "is_fixed":
                if is_output(state):
                    return "True"
                if type(call.args[0].get()) == IdExpr and call.args[0].get().is_var:
                    return "False"
                return "True"
            case "fix":
                return self.ast_expr(call.args[0], state)
            case "length":
                return f"len({self.ast_expr(call.args[0], state)})"
            case "ub" | "lb" | "ub_array" | "lb_array":
                return f"{call.id}({self.ast_expr(call.args[0], state)})"
            case "assert":
                return f"assert_({", ".join([self.ast_expr(arg, STATE_DEFAULT) for arg in call.args])})"
            case "log":
                return f"math.log({self.ast_expr(call.args[1], STATE_DEFAULT)})" 
            case "int2float":
                return f"float({self.ast_expr(call.args[0], STATE_DEFAULT)})" 
            case "bool2int":
                return f"bool2int(model, {self.ast_expr(call.args[0], STATE_CONSTRAINT_EXPR)})" 
            case "abs":
                if call.args[0].get().is_var:
                    return f"abs_(model, {self.ast_expr(call.args[0], state)})"

                return f"abs({self.ast_expr(call.args[0], STATE_DEFAULT)})" 
            case "forall":
                if is_constraint(state):
                    if is_stmt(state):
                        return self.ast_Comprehension(call.args[0].get(), state)

                    return f"forall_(model, {self.ast_expr(call.args[0], STATE_CONSTRAINT_EXPR)})"

                return f"all({self.ast_expr(call.args[0], STATE_DEFAULT)})"
            case "fzn_all_different_int":
                return f"ortools_all_different(model, {self.ast_expr(call.args[0], STATE_DEFAULT)})"
            case "ortools_table_int" | "ortools_table_bool":
                return f"ortools_allowed_assignments(model, {self.ast_expr(call.args[0], STATE_DEFAULT)}, {self.ast_expr(call.args[1], STATE_DEFAULT)})"
            case "ortools_inverse":
                return f"ortools_inverse(model, {", ".join([self.ast_expr(arg, STATE_DEFAULT) for arg in call.args])})"
            case "fzn_cumulative":
                return f"ortools_cumulative(model, {", ".join([self.ast_expr(arg, STATE_DEFAULT) for arg in call.args])})"
            case "fzn_disjunctive_strict":
                return f"ortools_disjunctive_strict(model, {", ".join([self.ast_expr(arg, STATE_DEFAULT) for arg in call.args])})"
            case "fzn_disjunctive":
                return f"ortools_disjunctive(model, {", ".join([self.ast_expr(arg, STATE_DEFAULT) for arg in call.args])})"
            case "ortools_circuit":
                return f"ortools_circuit(model, {", ".join([self.ast_expr(arg, STATE_DEFAULT) for arg in call.args])})"
            case "ortools_subcircuit":
                return f"ortools_subcircuit(model, {", ".join([self.ast_expr(arg, STATE_DEFAULT) for arg in call.args])})"
            case "ortools_count_eq":
                return f"ortools_count_eq(model, {", ".join([self.ast_expr(arg, STATE_DEFAULT) for arg in call.args])})"
            case "ortools_count_eq_cst":
                return f"ortools_count_eq_cst(model, {", ".join([self.ast_expr(arg, STATE_DEFAULT) for arg in call.args])})"
            case "fzn_diffn":
                return f"ortools_diffn(model, {", ".join([self.ast_expr(arg, STATE_DEFAULT) for arg in call.args])})"
            case "fzn_diffn_nonstrict":
                return f"ortools_diffn_nonstrict(model, {", ".join([self.ast_expr(arg, STATE_DEFAULT) for arg in call.args])})"
            case "ortools_network_flow_cost":
                return f"ortools_network_flow_cost(model, {", ".join([self.ast_expr(arg, STATE_DEFAULT) for arg in call.args])})"
            case "ortools_network_flow":
                return f"ortools_network_flow(model, {", ".join([self.ast_expr(arg, STATE_DEFAULT) for arg in call.args])})"
            case "ortools_regular":
                return f"ortools_regular(model, {", ".join([self.ast_expr(arg, STATE_DEFAULT) for arg in call.args])})"
            case "show_int":
                return f"show_int({self.ast_expr(call.args[0],state)}, {self.ast_expr(call.args[1],state)})"
            case "if_then_else":
                if (call.id, state) not in self.to_generate:
                    self.to_generate.append((call.id, state))
                return f"{call.id}({", ".join([self.ast_expr(arg, STATE_CONSTRAINT_EXPR) for arg in call.args])})"
            case _:
                if (call.id, state) not in self.to_generate:
                    self.to_generate.append((call.id, state))
                return f"{call.id}({", ".join([self.ast_expr(arg, STATE_DEFAULT) for arg in call.args])})"

    def generate_LetIn(self, let_in, state):
        let_fn = ast.FunctionDef(let_in.id, args = ast.arguments(args=[]))

        for decl in let_in.declarations:
            if type(decl) == DeclConst:
                let_fn.body.append(self.ast_const(decl))

            elif type(decl) == DeclVariable:
                let_fn.body.append(self.ast_var(decl))

        for constraint in let_in.constraints:
            let_fn.body.append(ast.parse(self.ast_expr(constraint, STATE_CONSTRAINT)))
        
        in_expr = ast.Return(ast.parse(self.ast_expr(let_in.in_expr, state)).body[-1].value)
        let_fn.body.append(in_expr)

        return let_fn

    def ast_Function(self, function_id: str, state):
        if function_id not in self.tree.functions:
            return

        function = self.tree.functions[function_id]
        fn = ast.FunctionDef(function.id)

        # if function_id.endswith("_reif"):
        #     fn.args = ast.arguments(args =  [ast.arg(arg="model")] + [ast.arg(arg=fn_arg.id) for fn_arg in function.params[:-1]])
        # else:
        fn.args = ast.arguments(args = [ast.arg(arg=fn_arg.id) for fn_arg in function.params])


        # if function_id.endswith("_reif") and type(function.body.get()) == BinOp and function.body.get().kind == BinOp.OpKind.EQUIV: # should always be true for _reif
        #     fn.body = ast.parse(self.ast_expr(function.body.get().rhs, state)).body
        # else:
        self.let_in_scope.append([])
        fn.body = ast.parse(self.ast_expr(function.body, state)).body
        if is_expr(state):
            fn.body = [ ast.Return(fn.body[-1].value) ]

        for (let_in, let_state) in self.let_in_scope[-1]:
            let_fn = self.generate_LetIn(let_in, let_state)
            fn.body.insert(0, ast.fix_missing_locations(let_fn))

        self.ast_tree.body.insert(self.function_place, ast.fix_missing_locations(fn))
        self.let_in_scope.pop()


    def ast_Comprehension(self, compr: Comprehension, state):
        if is_stmt(state):
            if_py, current = self.ast_generator(compr.generators, state)
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
                current = current.orelse[-1]
            if ite.else_expr is not None:
                current.orelse.append(ast.parse(self.ast_expr(ite.else_expr, state)))
            return ast.unparse(ast.fix_missing_locations(if_py))
        else:
            current_orelse = ast.parse(self.ast_expr(ite.else_expr, state)).body[0].value
            for cond, body in ite.if_then[::-1]:
                current_cond = ast.parse(self.ast_expr(cond, state)).body[0].value
                current_body = ast.parse(self.ast_expr(body, state)).body[0].value
                current_orelse = ast.IfExp(current_cond, current_body, current_orelse)

            return ast.unparse(ast.fix_missing_locations(current_orelse))


    def ast_generator(self, generators: list[Generator], state):
        if is_stmt(state):
            gen=ast.Module()
            current = gen
            for generator in generators:
                if type(generator) == Iterator:
                    iter = self.ast_expr(generator.in_expr, state if is_output(state) else STATE_DEFAULT)
                    if is_id_expr_array(generator.in_expr):
                        iter += '.values()'
                    current.body.append(ast.For(target=ast.Name(generator.variable.id), iter=ast.parse(iter).body[0].value))
                else:
                    current.body.append(ast.If(ast.parse(self.ast_expr(generator, state if is_output(state) else STATE_DEFAULT)).body[0].value))
                current = current.body[0]
            return gen, current
        else:
            gen=""
            for generator in generators:
                if type(generator) == Iterator:
                    iter = self.ast_expr(generator.in_expr, state)
                    if is_id_expr_array(generator.in_expr):
                        iter += '.values()'
                    gen+= f"for {generator.variable.id} in {iter} "
                else:
                    gen += f"if {self.ast_expr(generator, state)} "
            return gen

    def ast_declare_var(self, id: str, domain: ExprHandle):
        if domain is None:
            return f"model.new_int_var(-4611686018427387, 4611686018427387, {id})"

        return f"model.new_int_var_from_domain(cp_model.Domain.FromValues({self.ast_expr(domain, STATE_DEFAULT)}), {id})"

    def ast_var(self, decl: DeclVariable):
        if decl.value is not None:
            return ast.parse(f"{decl.id} = {self.ast_expr(decl.value, STATE_DEFAULT)}")
        match(decl.type.type()):
            case "int":
                return ast.parse(f"{decl.id} = {self.ast_declare_var(f"\"{decl.id}\"", decl.domain)}")
            case "bool":
                return ast.parse(f"{decl.id} = model.new_bool_var(\"{decl.id}\")")
            case "array":
                return ast.parse(f"{decl.id} = {self.ast_var_Array(decl, STATE_DEFAULT)}")
            case _:
                return ""
        #switch case which variable
    def ast_constraint(self, decl):
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

    def ast_LiteralSet(self, litset: LiteralSet, state):
        if len(litset.value) == 0:
            return "set()"

        return "{" + ", ".join([
            self.ast_expr(expr, state)
            for expr in litset.value
        ]) + "}"

    def ast_LetIn(self, let_in: LetIn, state):
        if is_stmt(state) and is_constraint(state):
            state = STATE_CONSTRAINT_EXPR

        self.let_in_scope[-1].append((let_in, state))

        return f"{let_in.id}()"

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
            emitter.ast_tree.body.append(emitter.ast_const(decl))

        elif type(decl) == DeclVariable:
            emitter.ast_tree.body.append(emitter.ast_var(decl))

    for constraints in tree.constraints:
        emitter.ast_constraint(constraints)

    emitter.ast_model_solve_type(tree.solve_type)
    emitter.ast_output(tree.output)

    i = 0

    for let_fn, let_state in emitter.let_in_scope[0]:
        result_fn = emitter.generate_LetIn(let_fn, let_state)
        emitter.ast_tree.body.insert(emitter.function_place, ast.fix_missing_locations(result_fn))

    while i < len(emitter.to_generate):
        emitter.ast_Function(emitter.to_generate[i][0], emitter.to_generate[i][1])
        i += 1
    
    emitter.finalize_file()
    emitter.unparse_ast_tree(file_to_write)
    file_to_write.close()
