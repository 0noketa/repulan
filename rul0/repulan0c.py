
# typeless subset to x86 compiler


import re


class Repulan0:
    def __init__(self,
            entry_point="repulan0_main",
            sym_prefix="repulan0_",
            var_prefix="rul0_var_",
            extern_funcs=[],
            extern_procs=[],
            extern_vars=[],
            use_auto=False) -> None:
        self.sym_prefix = sym_prefix
        self.entry_point = entry_point
        self.var_prefix = var_prefix
        self.extern_funcs = extern_funcs.copy()
        self.extern_vars = extern_vars.copy()
        self.uses_auto = use_auto

    def compile(self, src: str):
        src2 = re.split("""(#|[0-9]+|\{|\}|\(|\)|\[|\]|\\\\|\+|\-|\*|\/|\%|\=\=|\!\=|\?\!|\?|\||\!|(?:|\=)[A-Za-z_]+[A-Za-z_0-9]*)""", src)

        enabled_features = set(["auto_vars"]) if self.uses_auto else set()
        lambda_bodys = []
        header = []
        body_header = []
        body = []
        footer = []

        header.extend([
            f"%define C_API_PREFIX {self.sym_prefix}",
            '%include "repulan0.inc"',
            f"global {self.entry_point}"
        ])

        body_header.extend([
            f"{self.entry_point}:",
            "    rul0_begin"
        ])

        comment = False
        branch_idx = 0
        list_dpt = 0
        args_dpt = 0
        stat = "code"
        lambda_types = ["flat"]
        lambda_idx = 0
        branch_stacks = [[]]
        branch_stacks2 = [[]]
        ctxs = []
        params = []
        vs = []

        def put(s):
            if len(lambda_bodys) == 0:
                body.append(s)
            else:
                lambda_bodys[-1].append(s)

        def is_param(name):
            return len(params) and name == params[-1]

        def var_addr(name):
            if name in vs and (is_param(name) or name not in self.extern_vars):
                if self.uses_auto:
                    return f"ebp+({vs.index(name) + 1}*INT_SIZE)"
                else:
                    return f"{self.var_prefix}{name}"
            elif name in self.extern_vars:
                return name

        def push_branches():
            branch_stacks.append([])
            branch_stacks2.append([])
        def pop_branches():
            for i in branch_stacks.pop():
                put(f".rul0_else_{i}:")
                put(f".rul0_endif_{i}:")
            for i in branch_stacks2.pop():
                put(f".rul0_endif_{i}:")


        for tkn in src2:
            tkn = tkn.strip()
            if tkn == "":
                continue

            if tkn == "#":
                comment = not comment
                continue

            if comment:
                continue

            if stat == "lambda_head":
                if tkn == "{":
                    stat = "code"

                    if params[-1] not in vs:
                        vs.append(params[-1])

                    if lambda_types[-1] == "for":
                        put("    begin_param_stack")
                        put("    pop edx")
                        put(f"    mov eax, [{var_addr(params[-1])}]")
                        put("    push eax")
                        put("    end_param_stack")
                        put("    xor eax, eax")
                        put("    begin_temp_stack")
                        put("    push edx")
                        put("    push eax")
                        # while temp[-1] < param[-2] 
                        put("    pop eax")
                        put("    pop edx")
                        put("    end_temp_stack")
                        put(f"    mov [{var_addr(params[-1])}], eax")
                        put("    begin_temp_stack")
                        put("    push edx")
                        put("    push eax")
                        put("    end_temp_stack")
                        put("    cmp eax, edx")
                        put("    jge .rul0_loop_end")
                        put(".rul0_loop_begin:")
                    else:
                        put("    begin_param_stack")
                        put("    pop edx")
                        put(f"    xchg edx, [{var_addr(params[-1])}]")
                        put("    push edx")
                        put("    end_param_stack")
                    
                elif len(params) < len(ctxs):
                    params.append(tkn)
                else:
                    raise Exception(f"lambda has {params[-1]}. second param {tkn} is not valid.")
                continue
            if stat == "code":
                if tkn == "}":
                    if len(lambda_bodys) == 0:
                        raise Exception("unbalanced block")

                    pop_branches()

                    if lambda_types[-1] == "for":
                        put("    begin_temp_stack")
                        put("    pop eax")
                        put("    pop edx")
                        put("    end_temp_stack")
                        put("    inc eax")
                        put(f"    mov [{var_addr(params[-1])}], eax")
                        put("    begin_temp_stack")
                        put("    push edx")
                        put("    push eax")
                        put("    end_temp_stack")
                        put("    cmp eax, edx")
                        put("    jl .rul0_loop_begin")
                        put(".rul0_loop_end:")
                        put("    begin_temp_stack")
                        put("    pop eax")
                        put("    pop edx")
                        put("    end_temp_stack")
                    put("    param_pop edx")
                    put(f"    mov [{var_addr(params[-1])}], edx")
                    put("    end_data_stack")
                    put("    ret")

                    idx = ctxs.pop()
                    footer.extend(lambda_bodys.pop())
                    params.pop()
                    lambda_types.pop()

                    if len(ctxs) == 0:
                        stat = "code"

                    put(f"    push lambda_{idx}")

                    continue

            if tkn in ["\\", "for"]:
                stat = "lambda_head"
                lambda_types.append({"\\": "flat", "for": "for"}[tkn])
                ctxs.append(lambda_idx)
                lambda_bodys.append([])
                push_branches()
                put(f"lambda_{lambda_idx}:")
                put("    begin_data_stack")
                put(".recursion_target:")
                lambda_idx += 1
                continue

            if tkn in self.extern_funcs:
                put(f"    push wrapped_c_func_{tkn}")
                continue

            if tkn == "set":
                enabled_features.add("set")
                put("    push object_set")
                continue
            if tkn == "len":
                enabled_features.add("len")
                put("    push object_len")
                continue
            if tkn == "spread":
                enabled_features.add("spread")
                put("    push object_spread")
                continue
            if tkn == "reverse":
                enabled_features.add("reverse")
                put("    push object_reverse")
                continue
            if tkn == "del":
                enabled_features.add("del")
                put("    push object_del")
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
            if tkn == "restart":
                put(f"    rul0_restart_{lambda_types[-1]} {var_addr(params[-1])}")
                continue
            if tkn == "reserve_restart":
                put(f"    rul0_reserve_restart_{lambda_types[-1]}")
                continue

            if tkn.isidentifier():
                if tkn not in vs and tkn not in self.extern_vars:
                    vs.append(tkn)

                put(f"    push INT_TYPE_NAME[{var_addr(tkn)}]")
                continue

            if tkn.startswith("=") and tkn[1:].isidentifier():
                if tkn[1:] not in vs:
                    vs.append(tkn[1:])

                put(f"    pop INT_TYPE_NAME[{var_addr(tkn[1:])}]")
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
            if tkn == "*":
                put("    rul0_mul")
                continue
            if tkn == "/":
                put("    rul0_div")
                continue
            if tkn == "%":
                put("    rul0_mod")
                continue
            if tkn == "divmod":
                put("    rul0_divmod")
                continue
            if tkn == "==":
                put("    rul0_eq")
                continue
            if tkn == "!=":
                put("    rul0_neq")
                continue
            if tkn == ":":
                put("    rul0_range")
                continue
            if tkn == "(":
                push_branches()
                args_dpt += 1
                put("    begin_args")
                continue
            if tkn == ")":
                if args_dpt == 0:
                    raise Exception("')' has not its '('\n")
                args_dpt -= 1
                pop_branches()
                put("    rul0_call spreading_call")
                continue
            if tkn == "[":
                enabled_features.add("array")
                list_dpt += 1
                put("    begin_array")
                push_branches()
                continue
            if tkn == "]":
                enabled_features.add("array")
                if list_dpt == 0:
                    raise Exception("']' has not its '['\n")
                list_dpt -= 1
                pop_branches()
                put("    rul0_call alloc_array")
                continue
            if tkn == "?":
                put("    pop eax")
                put("    or eax, eax")
                put(f"    jz .rul0_else_{branch_idx}")
                branch_stacks[-1].append(branch_idx)
                branch_idx += 1
                continue
            if tkn == "|":
                if len(branch_stacks[-1]) == 0:
                    raise Exception("'|' has not its '?'\n")
                
                idx = branch_stacks[-1].pop()
                branch_stacks2[-1].append(idx)
                put(f"    jmp .rul0_endif_{idx}")
                put(f".rul0_else_{idx}:")
                continue
            if tkn == "?!":
                if len(branch_stacks2[-1]):
                    idx = branch_stacks2[-1].pop()
                else:
                    if len(branch_stacks[-1]):
                        idx = branch_stacks[-1].pop()
                        put(f".rul0_else_{idx}:")
                    else:
                        raise Exception("'!' has not its '|'\n")

                put(f".rul0_endif_{idx}:")
                continue

            raise Exception(f"{tkn} ?")

        for i in branch_stacks.pop():
            put(f".rul0_else_{i}:")
            put(f".rul0_endif_{i}:")
        for i in branch_stacks2.pop():
            put(f".rul0_endif_{i}:")

        if self.uses_auto:
            body.append("    rul0_dealloc_auto_vars")
            body_header.append(f"    rul0_alloc_auto_vars {len(vs)}")

        body.extend([
            "    rul0_end",
            "    ret"
        ])

        for i in enabled_features:
            header.insert(0, f"%define use_{i}")

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

    sym_prefix = "repulan0_"
    entry_point = "repulan0_main"
    extern_funcs = []
    extern_procs = []
    extern_vars = []
    use_auto = False

    for arg in sys.argv[1:]:
        if "=" in arg:
            val = arg[arg.index("=") + 1:]

        if arg in ["-?", "--help"]:
            print("  --sym_prefix=NAME   select prefix of function names")
            print("  --entry_point=NAME   select label name of entry_point")
            print("  --extern_func=NAME   declare external C func (uintptr_t(*)(uintptr_t))")
            print("  --extern_proc=NAME   declare external C func (void(*)(uintptr_t))")
            print("  --extern_var=NAME   declare external variable (uintptr_t)")
            print("  --auto   declare all variable as auto_variable (stub. do not use.)")

            sys.exit(0)

        if arg.startswith("--sym_prefix="):
            sym_prefix = val
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
            sym_prefix=sym_prefix,
            entry_point=entry_point,
            extern_funcs=extern_funcs,
            extern_procs=extern_procs,
            extern_vars=extern_vars,
            use_auto=use_auto)

    rul0.compile("".join(sys.stdin.readlines()))
