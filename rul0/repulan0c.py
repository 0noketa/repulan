
# typeless subset to x86 compiler

# available:
#   \x{}
#   f(x) f(x y z)
#   + - :
#   external C function (auto wrapping)


import re


def compile(src: str,
        entry_point="repulan0_main",
        extern_funcs=[],
        extern_vars=[],
        use_auto=False):
    src2 = re.split("""([0-9]+|\{|\}|\(|\)|\\\\|\+|\-|(?:|\=)[A-Za-z_]+[A-Za-z_0-9]*)""", src)

    lambda_bodys = []
    footer = ""

    print(f"""
%include "repulan0.inc"

global {entry_point}
{entry_point}:
""")

    if use_auto:
        print("""
    call repulan0_alloc_auto_vars
""")

    print("""
    repulan0_begin
""")

    stat = "main"
    lambda_idx = 0
    ctxs = []
    params = []
    vs = []

    def put(s):
        if stat == "main":
            print(s)
        else:
            lambda_bodys[-1] = lambda_bodys[-1] + s + "\n"

    def is_param(name):
        return len(params) and name == params[-1]

    def var_addr(name):
        if name in vs and (is_param(name) or name not in extern_vars):
            if use_auto:
                return f"ebp+({vs.index(name) + 1}*int_size)"
            else:
                return f"variable_{name}"
        elif name in extern_vars:
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
                footer += lambda_bodys.pop() + "\n"
                params.pop()

                if len(ctxs) == 0:
                    stat = "main"

                put(f"    push lambda_{idx}")

                continue

        if tkn == "\\":
            stat = "lambda_head"
            ctxs.append(lambda_idx)
            lambda_bodys.append("")
            put(f"lambda_{lambda_idx}:")
            put("    begin_data_stack")
            lambda_idx += 1
            continue

        if tkn in extern_funcs:
            put(f"    push wrapped_func_{tkn}")
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
            put("    repulan0_add")
            continue
        if tkn == "-":
            put("    repulan0_sub")
            continue
        if tkn == ":":
            put("    repulan0_range")
            continue
        if tkn == "(":
            put("    begin_args")
            continue
        if tkn == ")":
            put("    repulan0_call spreading_call")

            continue

    if use_auto:
        print(f"""
    repulan0_end
    call repulan0_dealloc_auto_vars
    ret
repulan0_alloc_auto_vars:
    begin_data_stack
    push ebp
    sub esp, ({len(vs)}*int_size)
    end_data_stack
    ret
repulan0_dealloc_auto_vars:
    begin_data_stack
    mov esp, ebp
    pop ebp
    end_data_stack
    ret
""")
    else:
        print(f"""
    repulan0_end
    ret
""")

    print(footer)

    if len(extern_funcs):
        print("extern " + ",".join(extern_funcs))

        for func_name in extern_funcs:
            print(f"""
wrapped_func_{func_name}:
    begin_data_stack
    param_pop edx

    repulan0_end

    push edx
    call {func_name}
    pop edx

    repulan0_begin

    push eax

    end_data_stack
    ret
""")

    if len(vs) and not use_auto:
        print("section .data")

        for i in vs:
            print(f"variable_{i}: dd 0")

    if len(extern_vars):
        print("extern " + ",".join(extern_vars))

if __name__ == "__main__":
    import sys

    entry_point = "repulan0_main"
    extern_funcs = []
    extern_vars = []
    use_auto = False

    for arg in sys.argv[1:]:
        if "=" in arg:
            val = arg[arg.index("=") + 1:]

        if arg in ["-?", "--help"]:
            print("  --entry_point=NAME   select label name of entry_point")
            print("  --extern_func=NAME   declare external C func (uintptr_t(*)(uintptr_t))")
            print("  --extern_var=NAME   declare external variable (uintptr_t)")
            print("  --auto   declare all variable as auto_variable")

            sys.exit(0)

        if arg.startswith("--entry_point="):
            entry_point = val
        if arg.startswith("--extern_func="):
            extern_funcs.append(val)
        if arg.startswith("--extern_var="):
            extern_vars.append(val)
        if arg.startswith("--auto"):
            use_auto = True

    compile("".join(sys.stdin.readlines()),
            entry_point=entry_point,
            extern_funcs=extern_funcs,
            extern_vars=extern_vars,
            use_auto=use_auto)
