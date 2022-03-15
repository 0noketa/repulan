#include <stdio.h>
#include <ctype.h>
#include "rplni.h"
#include "rplni_macro.h"
#include "rplni_loader.h"


int main()
{
    struct rplni_state state;

    if (!rplni_state_init(&state))
    {
        puts("failed to init");
        return 1;
    }

    struct rplni_scope *scope;
    struct rplni_value v;
    if (rplni_state_current_scope(&state, &scope))
    {
        //  \arg { arg __len } =len
        rplni_value_init_with_empty_func(&v, RPLNI_FUNC_FUNC, scope);
        rplni_func_add_param(v.value._func, "arg", 1);
#define prog (v.value._func->prog)
        CODE_S(RPLNI_OP_LOAD, "arg");
        CODE_I(RPLNI_OP_LEN, 0);
#undef prog
        rplni_scope_add_var(scope, "len");
        rplni_scope_store_var(scope, "len", &v);
        rplni_value_clean(&v, scope);


        //  \arg { "Hello, " arg "!" + + } =hello
        rplni_value_init_with_empty_func(&v, RPLNI_FUNC_FUNC, scope);
        rplni_func_add_param(v.value._func, "arg", 1);
        {
            char* src = "\"Hello, \" arg \"!\" + +";
            rplni_prog_load(&(v.value._func->prog),
                strlen(src), src,
                v.value._func->params,
                scope,
                NULL);
        }
        rplni_scope_add_var(scope, "hello");
        rplni_scope_store_var(scope, "hello", &v);
        rplni_value_clean(&v, scope);


        //  \arg {
        //      \x { x __print } =print
        //      \x { x __spread } =spread
        //      # new plan for multi-param funcs. every call simply takes args from queue.
        //      # expressivity was reduced but usable and easily implementable.
        //      [\x y z { x 10000 *  y 100 *  z  + + }(0 15:)] __print
        //      [hello(arg "asdfgh" "zxcvbn")] arg swap =arg
        //      arg swap arg
        //      { __print } =p  p call  p call  p call
        //      print([0 4:](2 1 0))
        //      [1 2 0 3] [dup(dup(dup(len(dup) 0 swap :)))] __print  drop  # apply copied permutation to it self twice
        //      "A" \x{ x "B" x + + } (dup dup) + + __print
        //      [spread([0 5:] [10 15:])] __print 
        //  } =main
        rplni_value_init_with_empty_func(&v, RPLNI_FUNC_FUNC, scope);
        rplni_func_add_param(v.value._func, "arg", 1);
        {
            char* src =
                " \\x { x __print } =print"
                " \\x { x __spread } =spread"
                " [\\x y z { x 10000 *  y 100 *  z  + + }(0 15:)] __print "
                " [hello(arg \"asdfgh\" \"zxcvbn\")] arg swap =arg"
                " arg swap arg"
                " { __print } =p  p call  p call  p call"
                " print([0 4:](2 1 0))"
                " [1 2 0 3] [dup(dup(dup(len(dup) 0 swap :)))] __print  drop"
                " \"A\" \\x{ x \"B\" x + + } (dup dup) + + __print"
                " [spread([0 5:] [10 15:])] __print ";
            rplni_prog_load(&(v.value._func->prog),
                strlen(src), src,
                v.value._func->params,
                scope,
                NULL);
        }
        rplni_scope_add_var(scope, "main");
        rplni_scope_store_var(scope, "main", &v);
        rplni_value_clean(&v, scope);


        /* arguments */
        RPLNI_DEF_STR(arg, "qwerty", scope);
        rplni_state_push_value(&state, &arg);
        rplni_value_clean(&arg, scope);

        rplni_scope_load_var(scope, "main", &v);
        rplni_func_run(v.value._func, &state);
        rplni_value_clean(&v, scope);
    }


    rplni_state_clean(&state);

#ifndef NDEBUG
    rplni_dump_leaked();
#endif
    return 0;
}
