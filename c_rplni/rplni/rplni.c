#include <stdio.h>
#include <ctype.h>
#include "rplni.h"
#include "rplni_macro.h"
#include "rplni_loader.h"

#define LINE_MAX 127
#define SRC_MAX (((LINE_MAX+1)*40)-1)


int main(int argc, char *argv[])
{
    struct rplni_state rplni;
    if (!rplni_state_init(&rplni))
    {
        puts("failed to init");
        return 1;
    }


    struct rplni_scope *scope;
    struct rplni_value v;
    if (rplni_state_current_scope(&rplni, &scope))
    {
        rplni_state_add_argv(&rplni, argc - 1, argv + 1);
 
        rplni_state_add_builtin_func(&rplni, "len", RPLNI_OP_LEN, 1);
        rplni_state_add_builtin_func(&rplni, "spread", RPLNI_OP_SPREAD, 1);
        rplni_state_add_builtin_func(&rplni, "sum", RPLNI_OP_SUM, 1);
        rplni_state_add_builtin_func(&rplni, "str", RPLNI_OP_STR, 1);
        rplni_state_add_builtin_func(&rplni, "int", RPLNI_OP_INT, 1);
        rplni_state_add_builtin_func(&rplni, "print", RPLNI_OP_PRINT, 1);
        rplni_state_add_builtin_func(&rplni, "input", RPLNI_OP_INPUT, 1);
        rplni_state_add_builtin_func(&rplni, "eval", RPLNI_OP_EVAL, 1);
        rplni_state_add_builtin_func(&rplni, "system", RPLNI_OP_SYSTEM, 1);
        rplni_state_add_builtin_func(&rplni, "optimize", RPLNI_OP_OPTIMIZE, 1);
        rplni_state_add_builtin_func(&rplni, "compile", RPLNI_OP_COMPILE, 3);

        fputs("reads source until empty line. and then evaluates it.\n", stderr);
        fputs("input [bye] to exit.\n", stderr);

        char src[SRC_MAX + 1] = {0,};
        char line[LINE_MAX + 1] = {0,};
        while (!feof(stdin))
        {
            fgets(line, LINE_MAX, stdin);
            if (!strcmp(line, "bye\n")) break;

            if (strlen(line) > 0 && !strcmp(line, "\n"))
            {
                struct rplni_scope tmp_scope;
                rplni_scope_init(&tmp_scope, &rplni);
                rplni_state_push_scope(&rplni, &tmp_scope);

                size_t size = strlen(src);
                rplni_state_eval(&rplni, size, src);
                src[0] = 0;

                rplni_state_pop_scope(&rplni, NULL);
                rplni_scope_clean(&tmp_scope, &rplni);

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


    rplni_state_clean(&rplni);

#ifndef NDEBUG
    rplni_dump_leaked();
#endif
    return 0;
}
