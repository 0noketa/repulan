
# typeless subset to x86 compiler

# available:
#   \x{}
#   f(x) f(x y z)
#   []
#   + - :
#   swap dup drop
#   external C function (auto wrapping)
# difference to Repulan:
#   [] allocates array and function staticallly bound to it. no GC exists.
#   arrays are unsafe when array element at invalid index was used.


import re


class Repulan0:
    def __init__(self,
            entry_point="repulan0_main",
            var_prefix="rul0_var_",
            extern_funcs=[],
            extern_procs=[],
            extern_vars=[],
            use_auto=False) -> None:
        self.entry_point = entry_point
        self.var_prefix = var_prefix
        self.extern_funcs = extern_funcs.copy()
        self.extern_vars = extern_vars.copy()
        self.uses_auto = use_auto

    def compile(self, src: str):
        src2 = re.split("""([0-9]+|\{|\}|\(|\)|\[|\]|\\\\|\+|\-|(?:|\=)[A-Za-z_]+[A-Za-z_0-9]*)""", src)

        lambda_bodys = []
        header = []
        body_header = []
        body = []
        footer = []

        if self.uses_auto:
            header.append("%define use_auto_vars")

        header.extend([
            '%include "repulan0.inc"',
            f"global {self.entry_point}"
        ])

        body_header.extend([
            f"{self.entry_point}:",
            "    rul0_begin"
        ])

        stat = "main"
        lambda_idx = 0
        ctxs = []
        params = []
        vs = []
        uses_array = False

        def put(s):
            if stat == "main":
                body.append(s)
            else:
                lambda_bodys[-1].append(s)

        def is_param(name):
            return len(params) and name == params[-1]

        def var_addr(name):
            if name in vs and (is_param(name) or name not in self.extern_vars):
                if self.uses_auto:
                    return f"ebp+({vs.index(name) + 1}*int_size)"
                else:
                    return f"{self.var_prefix}{name}"
            elif name in self.extern_vars:
                return name

        for tkn in src2:
            tkn = tkn.strip()
            if tkn == "":
                continue

            if stat == "lambda_head":
                if tkn == "{":
                    stat = "lambda_body"
                    if params[-1] not in vs:
                        vs.append(params[-1])

                    put("    param_pop edx")
                    put(f"    xchg edx, [{var_addr(params[-1])}]")
                    put("    param_push edx")
                elif len(params) < len(ctxs):
                    params.append(tkn)
                else:
                    raise Exception(f"lambda has {params[-1]}. second param {tkn} is not valid.")
                continue
            if stat == "lambda_body":
                if tkn == "}":
                    put("    param_pop edx")
                    put(f"    mov [{var_addr(params[-1])}], edx")
                    put("    end_data_stack")
                    put("    ret")

                    idx = ctxs.pop()
                    footer.extend(lambda_bodys.pop())
                    params.pop()

                    if len(ctxs) == 0:
                        stat = "main"

                    put(f"    push lambda_{idx}")

                    continue

            if tkn == "\\":
                stat = "lambda_head"
                ctxs.append(lambda_idx)
                lambda_bodys.append([])
                put(f"lambda_{lambda_idx}:")
                put("    begin_data_stack")
                lambda_idx += 1
                continue

            if tkn in self.extern_funcs:
                put(f"    push wrapped_c_func_{tkn}")
                continue

            if tkn == "len":
                uses_array = True
                put("    push array_len")
                continue
            if tkn == "swap":
                put("    rul0_suwap")
                continue
            if tkn == "dup":
                put("    rul0_dup")
                continue
            if tkn == "drop":
                put("    rul0_drop")
                continue
            if tkn == "deallocate_array":
                put("    push deallocate_array")
                continue

            if tkn.isidentifier():
                if tkn not in vs:
                    vs.append(tkn)

                put(f"    push int_type_name[{var_addr(tkn)}]")
                continue

            if tkn.startswith("=") and tkn[1:].isidentifier():
                if tkn[1:] not in vs:
                    vs.append(tkn[1:])

                put(f"    pop int_type_name[{var_addr(tkn[1:])}]")
                continue

            if tkn.isdigit():
                put(f"    push {int(tkn)}")
                continue
            if tkn == "+":
                put("    rul0_add")
                continue
            if tkn == "-":
                put("    rul0_sub")
                continue
            if tkn == ":":
                put("    rul0_range")
                continue
            if tkn == "(":
                put("    begin_args")
                continue
            if tkn == ")":
                put("    rul0_call spreading_call")
            if tkn == "[":
                uses_array = True
                put("    begin_array")
                continue
            if tkn == "]":
                uses_array = True
                put("    rul0_call alloc_array")
                continue

        if self.uses_auto:
            body.append("    rul0_dealloc_auto_vars")
            body_header.append(f"    rul0_alloc_auto_vars {len(vs)}")

        body.extend([
            "    rul0_end",
            "    ret"
        ])

        if uses_array:
            header.insert(0, "%define use_array")

        print("\n".join(header))
        print("\n".join(body_header))
        print("\n".join(body))
        print("\n".join(footer))

        if len(self.extern_funcs):
            print("extern " + ",".join(self.extern_funcs))

            for func_name in self.extern_funcs:
                print(f"wrap_c_func wrapped_c_func_{func_name}, {func_name}")

        if len(vs) and not self.uses_auto:
            print("section .data")

            for i in vs:
                print(f"{var_addr(i)}: dd 0")

        if len(self.extern_vars):
            print("extern " + ",".join(self.extern_vars))

if __name__ == "__main__":
    import sys

    entry_point = "repulan0_main"
    extern_funcs = []
    extern_procs = []
    extern_vars = []
    use_auto = False

    for arg in sys.argv[1:]:
        if "=" in arg:
            val = arg[arg.index("=") + 1:]

        if arg in ["-?", "--help"]:
            print("  --entry_point=NAME   select label name of entry_point")
            print("  --extern_func=NAME   declare external C func (uintptr_t(*)(uintptr_t))")
            print("  --extern_proc=NAME   declare external C func (void(*)(uintptr_t))")
            print("  --extern_var=NAME   declare external variable (uintptr_t)")
            print("  --auto   declare all variable as auto_variable")

            sys.exit(0)

        if arg.startswith("--entry_point="):
            entry_point = val
        if arg.startswith("--extern_func="):
            extern_funcs.append(val)
        if arg.startswith("--extern_proc="):
            extern_procs.append(val)
        if arg.startswith("--extern_var="):
            extern_vars.append(val)
        if arg.startswith("--auto"):
            use_auto = True


    rul0 = Repulan0(
            entry_point=entry_point,
            extern_funcs=extern_funcs,
            extern_procs=extern_procs,
            extern_vars=extern_vars,
            use_auto=use_auto)

    rul0.compile("".join(sys.stdin.readlines()))
