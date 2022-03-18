
# stack-based toy language for repunits

from typing import Callable, Dict, Union, List
import sys
import re


class RepunitBase:
    def __init__(self) -> None:
        pass
    def value(self, default_base: int = None):
        pass
    def update(self, cols: int = None, base: int = None):
        pass

class Repunit(RepunitBase):
    def __init__(self, cols: Union[int, str] = None, base: Union[int, RepunitBase] = None):
        super().__init__()
        self.cols = None
        self.base = None

        if type(cols) is str:
            cols: str = cols
            if cols.count("1") != len(cols):
                cols = 0
            else:
                cols = len(cols)
        self.update(cols, base)

    def __repr__(self) -> str:
        if self.cols is None:
            return f"?({self.base})"
        elif self.base is None:
            return "1" * self.cols + "(?)"
        else:
            return "1" * self.cols + f"({self.base})"

    def __eq__(self, __o: object) -> bool:
        if type(__o) == int and self.cols is not None:
            return self.value(default_base=1) == __o

        return isinstance(__o, Repunit) and self.base is __o.base and self.cols is __o.cols

    def value(self, default_base: int = None):
        base = self.base if self.base is not None else default_base

        if self.cols is None:
            return self

        if base is None:
            return self

        if self.cols == 0 or base == 0:
            return 0

        if self.cols < 0 or base < 0:
            raise Exception("repunit function with negative args is not implemented")

        if base == 1:
            return self.cols
        else:
            return (base ** self.cols - 1) // (base - 1)

    def update(self, cols: int = None, base: Union[int, RepunitBase] = None, force=False):
        if type(cols) is str:
            cols = int(cols)
        if type(base) is str:
            base = int(base)

        if cols is not None and (force or self.cols is None):
            self.cols = cols

        if isinstance(base, Repunit):
            base = base.value(default_base=1)

        if base is not None and (force or self.base is None):
            self.base = base

class EmptyValue:
    def __init__(self) -> None:
        pass

class RepulanBase:
    def __init__(self):
        pass

    def eval(self, src: str, param_names: List[str] = None, param_vals: list = None, scope: dict = None):
        pass

class QuotedCode:
    def __init__(self, params: List[str], body: List[str], scope: dict, lambda_type: str = "lambda", interp: RepulanBase = None) -> None:
        self.params = params
        self.body = body
        self.scope = scope
        self.lambda_type = lambda_type
        self.interp = interp

    def eval(self, *args):
        """wraps as Python's callable"""

        idx = len(self.interp.stack)

        if self.lambda_type.startswith("foreach"):
            for arg in args:
                q = [arg]

                while len(q):
                    for i in q[0]:
                        q2 = self.interp.eval(self.body, self.params, [i], self.scope)

                        if len(q2):
                            q.extend(q2)
                            break

                    q = q[1:]
        elif self.lambda_type.startswith("for"):
            for arg in args:
                q = [arg]

                while len(q):
                    for i in range(self.interp.get_int(q[0])):
                        q2 = self.interp.eval(self.body, self.params, [i], self.scope)
                        if len(q2):
                            q.extend(q2)
                            break

                    q = q[1:]
        else:
            q = [args]

            while len(q):
                q2 = self.interp.eval(self.body, self.params, q[0], self.scope)
                if len(q2):
                    q.extend(q2)

                q = q[1:]

        if idx >= len(self.interp.stack):
            result = EmptyValue()
        else:
            result = self.interp.stack.pop()

        return result


class Repulan(RepulanBase):
    def __init__(self, named_vars: dict = {}, native_funcs: Dict[str, Callable] = {}, use_repunits = True):
        super().__init__()
        self.stack = []
        self.uses_repunits = use_repunits

        self.named_vars = {}
        self.native_funcs = {
            "repeat": (lambda x: (lambda y: x * y)),
            "pass": (lambda x: (lambda y: self.repulan_call(y, x))),
            "spread": self.nativefunc_spread,
            "pop": self.nativefunc_pop,
            "repunit": Repunit,
            "reverse": (lambda x: (list if type(x) is list else "".join)(reversed(x))),
            "len": len,
            "sort": (lambda x: list(sorted(x))),
            "unique": (lambda x: list(sorted(set(x)))),
            "join": (lambda x: x.join),
            "compose": (lambda x: (lambda y: self.nativefunc_compose(x, y))),
            "iif": (lambda x: (lambda y: (lambda z: y if x else z))),
            "int": self.get_int,
            "str": str
        }

        self.named_vars.update(named_vars)
        self.native_funcs.update(native_funcs)

    def add_func(self, name, callable: Callable, no_overwrite=False):
        if no_overwrite or name not in self.native_funcs:
            self.native_funcs[name] = callable

    def add_wrapped_func(self, name, callable: Callable, no_overwrite=False):
        if no_overwrite or name not in self.native_funcs:
            self.add_func(name, self.repulan_wrap_func(callable))

    def repulan_wrap_func(self, callable: Callable):
        return (lambda x: self.call_and_drop(callable, x))

    def repulan_call(self, func, arg):
        # sys.stderr.write(f"{func}({arg})  {self.stack[-8:]}\n")
        if isinstance(func, Repunit):
            func.update(base=arg, force=True)

            return func.value()

        if type(func) in [int, float]:
            return func * self.get_int(arg)
        elif type(func) in [list, str]:
            if len(func) == 0:
                return func([])
            return func[self.get_int(arg) % len(func)]
        else:
            return func(arg)

    def repulan_spreading_call(self, func, arg0_idx):
        idx = arg0_idx

        if len(self.stack) < idx:
            raise Exception("function argument can not reduce stack elements")

        args = self.stack[idx:]
        self.stack = self.stack[:idx]


        for arg in args:
            result = self.repulan_call(func, arg)
            if not isinstance(result, EmptyValue):
                self.stack.append(result)

        # sys.stderr.write(f"2--{self.stack}\n")

    def nativefunc_compose(self, f0, f1):
        # sys.stderr.write(f"compose({f0})({f1})\n")
        # sys.stderr.write(f"  {self.stack}\n")

        def f(x):
            idx = len(self.stack)

            self.stack.append(x)
            self.repulan_spreading_call(f1, idx)
            self.repulan_spreading_call(f0, idx)

            if idx < len(self.stack):
                return self.stack.pop()
            else:
                return EmptyValue()

        return f

    def call_and_drop(self, f, x):
        """wraps functions that return None"""
        self.repulan_call(f, x)

        return EmptyValue()

    def nativefunc_spread(self, x):
        if type(x) is list:
            self.stack.extend(x)

        return EmptyValue()

    def nativefunc_push(self, x):
        self.stack.append(x)

        return EmptyValue()

    def nativefunc_pop(self, x):
        return self.stack.pop(len(self.stack) - self.get_int(x) - 1)


    def get_value(self, x):
        if isinstance(x, Repunit):
            return x.value(default_base=1)

        return x

    def get_int(self, x):
        x = self.get_value(x)

        if type(x) in [list, str]:
            x = len(x)

        return x

    def eval(self, src: Union[str, List[str]], param_names: List[str] = None, param_vals: list = None, scope: dict = None) -> list:
        """returns restart queue"""

        if scope is not None:
            old_scope = {}

            for name in scope:
                if name in self.named_vars:
                    old_scope[name] = self.named_vars[name]

                self.named_vars[name] = scope[name]

        if param_vals is not None:
            old_vars = {}

            for name in param_names:
                if name in self.named_vars:
                    old_vars[name] = self.named_vars[name]

            for i, arg in enumerate(param_vals):
                self.named_vars[param_names[i]] = arg

        def update():
            if param_vals is not None:
                for name in param_names:
                    self.named_vars.pop(name)

                    if name in old_vars:
                        self.named_vars[name] = old_vars[name]

            if scope is not None:
                for name in scope:
                    scope[name] = self.named_vars[name]

                    self.named_vars.pop(name)

                    if name in old_scope:
                        self.named_vars[name] = old_scope[name]


        if type(src) is str:
            if self.uses_repunits:
                ptn = """(#[^#]*#|1+|\.|\+|\-|\*|\>|\<|\/|\(|\)\[|\]|\\\\|\{|\}|/?|\=\=|!\=|\:|\?\!|/?|\||\!|(?:|=)[A-Za-z_]+[A-Za-z_0-9]*|"(?:\\\\.|[^\\\\"])*"|'(?:\\\\.|[^\\\\'])*')"""
            else:
                ptn = """(#[^#\n]*(?:#|\n)|\\d+|\.|\+|\-|\*|\>|\<|\/|\(|\)\[|\]|\\\\|\{|\}|/?|\=\=|!\=|\:|\?\!|/?|\||\!|(?:|=)[A-Za-z_]+[A-Za-z_0-9]*|"(?:\\\\.|[^\\\\"])*"|'(?:\\\\.|[^\\\\'])*')"""

            src2: List[str] = re.split(ptn, src)
        else:
            src2 = src

        reserved_restart_args = []
        app_stack = []
        list_stack = []
        block_depth = 0
        lambda_type = "lambda"
        lambda_param_buf = []
        lambda_body_buf = []
        state = "code"
        branch_dpt = 0

        for tkn in src2:
            if tkn.strip() == "":
                continue

            if tkn.startswith("#"):
                continue

            # print(f"tkn: {tkn:4}  {self.stack}")

            if state == "args":
                if tkn == "\\":
                    lambda_type += tkn
                elif tkn == "{":
                    state = "body"
                else:
                    lambda_param_buf.append(tkn)

                continue

            if state == "body":
                if tkn == "}" and block_depth == 0:
                    if len(lambda_type) > 1 and lambda_type.endswith("\\"):
                        lambda_scope = {key: self.named_vars[key] for key in self.named_vars
                                        if key not in lambda_param_buf
                                                and (key in lambda_body_buf or f"={key}" in lambda_body_buf)}

                        for lambda_tkn in lambda_body_buf:
                            if not lambda_tkn.startswith("="):
                                continue

                            lambda_tkn = lambda_tkn[1:]

                            if lambda_tkn.isidentifier() and lambda_tkn not in lambda_scope:
                                lambda_scope[lambda_tkn] = 1
                    else:
                        lambda_scope = None

                    self.stack.append(QuotedCode(lambda_param_buf, lambda_body_buf, lambda_scope, lambda_type, self).eval)

                    lambda_param_buf = []
                    lambda_body_buf = []

                    state = "code"
                else:
                    lambda_body_buf.append(tkn)

                    if tkn == "{":
                        block_depth += 1
                    elif tkn == "}":
                        block_depth -= 1

                continue


            if state == "skip_then":
                if tkn in ["?", "(", "[", "{"]:
                    branch_dpt += 1
                    continue
                elif tkn in ["?!", ")", "]", "}"]:
                    if branch_dpt == 0:
                        state = "code"
                        # block tail should be evaluated
                        if tkn == "?!":
                            continue
                    else:
                        branch_dpt -= 1
                        continue
                elif tkn == "|":
                    if branch_dpt == 0:
                        state = "code"
                    continue
                else:
                    continue
            if state == "skip_else":
                if tkn in ["?", "(", "[", "{"]:
                    branch_dpt += 1
                    continue
                elif tkn in ["?!", ")", "]", "}"]:
                    if branch_dpt == 0:
                        state = "code"
                        # block tail should be evaluated
                        if tkn == "?!":
                            continue
                    else:
                        branch_dpt -= 1
                        continue
                else:
                    continue

            if tkn == "\\":
                lambda_type = "lambda"
                state = "args"

                continue

            if tkn == "for":
                lambda_type = "for"
                state = "args"

                continue

            if tkn == "foreach":
                lambda_type = "foreach"
                state = "args"

                continue


            if tkn == "?":
                cond = self.stack.pop()
                if cond == 0:
                    state = "skip_then"
                continue
            if tkn == "|":
                state = "skip_else"
                continue
            if tkn == "?!":
                continue

            if tkn == "dup":
                x = self.stack.pop()
                self.stack.append(x)
                self.stack.append(x)
                continue
            if tkn == "swap":
                y = self.stack.pop()
                x = self.stack.pop()
                self.stack.append(y)
                self.stack.append(x)
                continue
            if tkn == "drop":
                self.stack.pop()
                continue

            if tkn in self.named_vars:
                self.stack.append(self.named_vars[tkn])
                continue
            if tkn.startswith("=") and tkn[1:].isidentifier():
                self.named_vars[tkn[1:]] = self.stack.pop()
                continue
            if tkn in self.native_funcs:
                self.stack.append(self.native_funcs[tkn])
                continue
            if tkn == ":":
                to_ = self.get_int(self.stack.pop())
                from_ = self.get_int(self.stack.pop())

                if from_ < to_:
                    self.stack += list(range(int(from_), int(to_)))
                else:
                    self.stack += list(range(int(from_), int(to_), -1))
                continue
            if self.uses_repunits and tkn.startswith("1"):
                self.stack.append(Repunit(len(tkn)))
                continue
            if not self.uses_repunits and tkn.isdigit():
                self.stack.append(int(tkn))
                continue
            if tkn.startswith('"'):
                # self.stack.append(tkn[1:-1])
                self.stack.append(eval(tkn))
                continue
            if tkn == "(":
                f = self.stack.pop()
                app_stack.append(f)
                app_stack.append(len(self.stack))
                continue
            if tkn == ")":
                idx = app_stack.pop()
                f = app_stack.pop()
                self.repulan_spreading_call(f, idx)
                continue
            if tkn == "[":
                list_stack.append(len(self.stack))
                continue
            if tkn == "]":
                idx = list_stack.pop()
                lst = [self.stack.pop() for _ in range(len(self.stack) - idx)]
                lst.reverse()

                self.stack.append(lst)
                continue

            if tkn == "restart":
                return [[self.stack.pop()]]

            if tkn == "reserve_restart":
                x = self.stack.pop()

                reserved_restart_args.append([x])
                continue

            if tkn == "==":
                y = self.stack.pop()
                x = self.stack.pop()

                self.stack.append(int(x == y))
                continue

            if tkn == "!=":
                y = self.stack.pop()
                x = self.stack.pop()

                self.stack.append(int(x != y))
                continue

            if tkn == "<":
                y = self.stack.pop()
                x = self.stack.pop()

                self.stack.append(int(x < y))
                continue

            if tkn == ">":
                y = self.stack.pop()
                x = self.stack.pop()

                self.stack.append(int(x > y))
                continue

            if tkn in ["+", "-", "*", "/", "%"]:
                y = self.get_value(self.stack.pop())
                x = self.get_value(self.stack.pop())

                if tkn == "+":
                    self.stack.append(x + y)
                if tkn == "-":
                    if type(x) is str and type(y) is str:
                        self.stack.append(x.replace(y, ""))
                    elif type(x) is list and callable(y):
                        self.stack.append([i for i in x if self.get_int(y(i)) == 0])
                    elif type(x) is list:
                        self.stack.append([i for i in x if i != y])
                    else:
                        self.stack.append(x - y if x >= y else 0)
                if tkn == "*":
                    if type(x) is list and type(y) is str:
                        self.stack.append(y.join(map(str, x)))
                    elif type(x) is list and callable(y):
                        self.stack.append(list(map(y, x)))
                    elif type(x) is int and type(y) is int:
                        self.stack.append(x * y)
                    else:
                        self.stack.append(self.nativefunc_compose(x, y))
                if tkn == "/":
                    if type(x) is str and type(y) is str:
                        self.stack.append(x.split(y))
                    else:
                        self.stack.append(x // y if y != 0 else 0)
                if tkn == "%":
                    self.stack.append(x % y if y != 0 else 0)
                continue

            sys.stderr.write(f"undefined {tkn} was ignored\n")


        update()

        return reserved_restart_args



if __name__ == "__main__":
    import io
    import os

    use_repunits = False

    if len(sys.argv) > 1:
        if sys.argv[1].endswith(".rul"):
            use_repunits = True

        if sys.argv[1] != "-":
            with io.open(sys.argv[1], "r") as f:
                src = " ".join(f.readlines())
        else:
            src = " ".join(sys.stdin.readlines())

        repulan_argv = sys.argv[2:]
    else:
        src = " ".join(sys.stdin.readlines())
        repulan_argv = []

    repulan = Repulan(named_vars={"argv": repulan_argv}, use_repunits=use_repunits)

    loaded_files = []

    def nativefunc_import_file(file_name):
        global repulan

        src2 = ""

        if not os.path.isfile(file_name):
            dir_name = os.path.dirname(os.path.abspath(sys.argv[1]))
            file_name = os.path.join(dir_name, "lib", file_name)

        if not os.path.isfile(file_name) or file_name in loaded_files:
            return EmptyValue()

        try:
            with io.open(file_name, "r") as f:
                src2 = " ".join(f.readlines())

            loaded_files.append(file_name)
            repulan.eval(src2)
        except:
            pass

        return EmptyValue()

    def nativefunc_python(s):
        ptn = """(\$\{[a-zA-Z_]+[a-zA-Z_0-9]*\})"""
        s2 = ""
        for i in re.split(ptn, s):
            if re.match(ptn, i):
                s2 += f'repulan_vars["{i[2:-1]}"]'
            else:
                s2 += i

        exec(s2, {
                "repulan_vars": repulan.named_vars,
                "repulan_int": repulan.get_int,
                "repulan_wrap_void_func": repulan.repulan_wrap_func
            })

        return EmptyValue()


    repulan.add_wrapped_func("print", print)
    repulan.add_func("input", input)
    repulan.add_func("import", nativefunc_import_file)
    repulan.add_func("eval", repulan.repulan_wrap_func(repulan.eval))
    repulan.add_func("system", nativefunc_python)

    repulan.eval(src)

    if len(repulan.stack):
        print(repulan.stack)
