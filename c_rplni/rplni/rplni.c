#include <stdio.h>
#include <ctype.h>
#include "rplni.h"
#include "rplni_macro.h"
#include "rplni_loader.h"

#define LINE_MAX 127
#define SRC_MAX (((LINE_MAX+1)*20)-1)


int main(int argc, char *argv[])
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
        v.type = RPLNI_LIST;
        v.value._list = rplni_list_new(argc, scope);
        for (int i = 1; i < argc; ++i)
        {
            struct rplni_value arg;
            rplni_value_init_with_cstr(&arg, argv[i], scope);
            rplni_list_push(v.value._list, &arg);
            rplni_value_clean(&arg, scope);
        }
        rplni_scope_add_var(scope, "argv");
        rplni_scope_store_var(scope, "argv", &v);
        rplni_value_clean(&v, scope);

 
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

        //  \arg { arg __spread } =spread
        rplni_value_init_with_empty_func(&v, RPLNI_FUNC_FUNC, scope);
        rplni_func_add_param(v.value._func, "arg", 1);
#define prog (v.value._func->prog)
        CODE_S(RPLNI_OP_LOAD, "arg");
        CODE_I(RPLNI_OP_SPREAD, 0);
#undef prog
        rplni_scope_add_var(scope, "spread");
        rplni_scope_store_var(scope, "spread", &v);
        rplni_value_clean(&v, scope);

        //  \arg { arg __str } =str
        rplni_value_init_with_empty_func(&v, RPLNI_FUNC_FUNC, scope);
        rplni_func_add_param(v.value._func, "arg", 1);
#define prog (v.value._func->prog)
        CODE_S(RPLNI_OP_LOAD, "arg");
        CODE_I(RPLNI_OP_STR, 0);
#undef prog
        rplni_scope_add_var(scope, "str");
        rplni_scope_store_var(scope, "str", &v);
        rplni_value_clean(&v, scope);

        //  \arg { arg __print } =print
        rplni_value_init_with_empty_func(&v, RPLNI_FUNC_FUNC, scope);
        rplni_func_add_param(v.value._func, "arg", 1);
#define prog (v.value._func->prog)
        CODE_S(RPLNI_OP_LOAD, "arg");
        CODE_I(RPLNI_OP_PRINT, 0);
#undef prog
        rplni_scope_add_var(scope, "print");
        rplni_scope_store_var(scope, "print", &v);
        rplni_value_clean(&v, scope);


        fputs("reads source until empty line. and then evaluates it.\n", stderr);
        fputs("input [bye] to exit.\n", stderr);

        char src[SRC_MAX + 1] = {0,};
        char line[LINE_MAX + 1] = {0,};
        while (!feof(stdin))
        {
            fgets(line, LINE_MAX, stdin);
            if (!strcmp(line, "bye\n")) break;

            if (!strcmp(line, "\n"))
            {
                struct rplni_scope tmp_scope;
                rplni_scope_init(&tmp_scope);
                rplni_state_push_scope(&state, &tmp_scope);

                size_t size = strlen(src);
                rplni_state_eval(&state, size, src, &tmp_scope);
                src[0] = 0;

                rplni_state_pop_scope(&state, NULL);
                rplni_scope_clean(&tmp_scope);

                continue;
            }

            size_t old_size = strlen(src);
            size_t line_size = strlen(line);
            size_t new_size = old_size + line_size;
            if (new_size > SRC_MAX) new_size = SRC_MAX;

            strncpy(src + old_size, line, new_size - old_size);
            src[new_size] = 0;
        }
    }


    rplni_state_clean(&state);

#ifndef NDEBUG
    rplni_dump_leaked();
#endif
    return 0;
}
