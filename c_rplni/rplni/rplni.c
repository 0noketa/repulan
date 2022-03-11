#include <stdio.h>
#include <ctype.h>
#include "rplni.h"
#include "rplni_macro.h"


int main()
{
    struct rplni_state state;

    if (!rplni_state_init(&state))
    {
        puts("failed to init");
        return 1;
    }

    /* global variable */
    if (!rplni_scope_add_var(&state.scope, "var0"))
    {
        puts("failed to add variable");

        rplni_scope_clean(&state.scope);
        return 1;
    }

    struct rplni_scope *scope;
    if (rplni_state_current_scope(&state, &scope))
    {
         RPLNI_DEF_FUNC(hello_func, RPLNI_FUNC_BLOCK, scope);
         rplni_func_add_param(hello_func.value._func, "arg");
#define prog (hello_func.value._func->prog)
         CODE_S(RPLNI_OP_PUSH, "Hello, ");
         CODE_S(RPLNI_OP_LOAD, "arg");
         CODE_S(RPLNI_OP_PUSH, "!");
         CODE_I(RPLNI_OP_ADD, 0);
         CODE_I(RPLNI_OP_ADD, 0);
#undef prog

        RPLNI_DEF_FUNC(main_func, RPLNI_FUNC_BLOCK, scope);
        rplni_func_add_param(main_func.value._func, "arg");
#define prog (main_func.value._func->prog)

        CODE_I(RPLNI_OP_BEGIN_LIST, 0);
        CODE_V(RPLNI_OP_PUSH, &hello_func);
        CODE_I(RPLNI_OP_BEGIN_ARGS, 0);
        CODE_S(RPLNI_OP_LOAD, "arg");
        CODE_S(RPLNI_OP_PUSH, "asdfgh");
        CODE_S(RPLNI_OP_PUSH, "zxcvbn");
        CODE_I(RPLNI_OP_SPREAD_CALL, 0);
        CODE_I(RPLNI_OP_END_LIST, 0);

        CODE_S(RPLNI_OP_LOAD, "arg");
        CODE_I(RPLNI_OP_SWAP, 0);

        CODE_S(RPLNI_OP_STORE, "arg");

        CODE_S(RPLNI_OP_LOAD, "arg");
        CODE_I(RPLNI_OP_SWAP, 0);
        CODE_S(RPLNI_OP_LOAD, "arg");

        CODE_I(RPLNI_OP_PRINT, 0);
        CODE_I(RPLNI_OP_PRINT, 0);
        CODE_I(RPLNI_OP_PRINT, 0);
#undef prog

        /* arguments */
        RPLNI_DEF_STR(arg, "qwerty", scope);
        rplni_state_push_value(&state, &arg);

        rplni_func_run(main_func.value._func, &state);

        rplni_value_clean(&hello_func, NULL);
        rplni_value_clean(&main_func, NULL);
    }


    rplni_state_clean(&state);
    return 0;
}
