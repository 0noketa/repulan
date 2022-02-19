
# typeless subset to x86 compiler

# available:
#   \x{}
#   f(x) f(x y z)
#   + - :
#   external C function (auto wrapping)
# limitation:
#   lambdas can use only parameter and global variables.


import re


def compile(src: str,
        entry_point="repulan0_main",
        extern_funcs=[],
        extern_vars=[]):
    src2 = re.split("""([0-9]+|\{|\}|\(|\)|\\\\|\+|\-|(?:|\=)[A-Za-z_]+[A-Za-z_0-9]*)""", src)

    lambda_bodys = []
    footer = ""

    print(f"""
%include "repulan0.inc"

global {entry_point}
{entry_point}:
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

    for tkn in src2:
        tkn = tkn.strip()
        if tkn == "":
            continue

        if stat == "lambda_head":
            if tkn == "{":
                stat = "lambda_body"
            elif len(params) < len(ctxs):
                params.append(tkn)
            else:
                raise Exception(f"lambda has {params[-1]}. second param {tkn} is not valid.")
            continue
        if stat == "lambda_body":
            if tkn == "}":
                put("    param_pop edx")
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

        if len(params):
            if tkn == params[-1]:
                put("    param_pop eax")
                put("    param_push eax")
                put("    push eax")
                continue
            if tkn.startswith("=") and tkn[1:] == params[-1]:
                put("    param_pop eax")
                put("    pop eax")
                put("    param_push eax")
                continue

        if tkn in extern_funcs:
            put(f"    push wrapped_func_{tkn}")
            continue

        if tkn.isidentifier():
            if tkn not in vs:
                vs.append(tkn)

            if tkn in extern_vars:
                pfx = ""
            else:
                pfx = "variable_"

            put(f"    push int_type_name[{pfx}{tkn}]")
            continue

        if tkn.startswith("=") and tkn[1:].isidentifier():
            if tkn[1:] not in vs:
                vs.append(tkn[1:])

            put(f"    pop int_type_name[variable_{tkn[1:]}]")
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

    if len(vs):
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

    for arg in sys.argv[1:]:
        if "=" in arg:
            val = arg[arg.index("=") + 1:]

        if arg in ["-?", "--help"]:
            print("  --entry_point=NAME   select label name of entry_point")
            print("  --extern_func=NAME   declare external C func (uintptr_t(*)(uintptr_t))")
            print("  --extern_var=NAME   declare external variable (uintptr_t)")

            sys.exit(0)

        if arg.startswith("--entry_point="):
            entry_point = val
        if arg.startswith("--extern_func="):
            extern_funcs.append(val)
        if arg.startswith("--extern_var="):
            extern_vars.append(val)

    compile("".join(sys.stdin.readlines()),
            entry_point=entry_point,
            extern_funcs=extern_funcs,
            extern_vars=extern_vars)
