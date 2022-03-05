#include <stdio.h>
#include <ctype.h>
#include "rplni.h"
#include "rplni_macro.h"


// interpreter for partial debugging
int main()
{
    int initialization_error = 0;
    struct rplni_scope global_scope;
    struct rplni_scope scope;
    if (!rplni_scope_init(&global_scope)) initialization_error = 1;
    if (!initialization_error && !rplni_scope_init(&scope))
    {
        rplni_scope_clean(&global_scope);

        initialization_error = 1;
    }

    if (initialization_error)
    {
        puts("failed to init scope");
        return 1;
    }

    struct rplni_list* data_stack = rplni_list_new(32, &scope);
    struct rplni_list* list_head_stack = rplni_list_new(32, &scope);
    if (!data_stack || !list_head_stack)
    {
        if (data_stack) rplni_list_unref(data_stack, &scope);
        if (list_head_stack) rplni_list_unref(data_stack, &scope);

        puts("failed to alloc lists");
        return 1;
    }

    RPLNI_DEF(x);
    RPLNI_DEF(y);

    while (!feof(stdin))
    {
        int cmd = getchar();

        if (strchr(" \t\n\r", cmd)) continue;

        switch (cmd)
        {
        case '[':
            // [ (--)
            x.type = RPLNI_UINT;
            x.value._uint = data_stack->size;
            rplni_list_push(list_head_stack, &x, &scope);
            break;
        case ']':
            // ] (-- list)
            if (list_head_stack->size == 0)
            {
                fputs("operator(]) failed: ([) was not used\n", stderr);
                break;
            }
            rplni_list_pop(list_head_stack, &x, &scope);
            y.type = RPLNI_LIST;
            {
                size_t size = data_stack->size - x.value._uint;
                y.value._list = rplni_list_new_with_captured(size, data_stack, &scope);
                rplni_list_push(data_stack, &y, &scope);
            }
            break;
        case '+':
            // + (x y -- x+y)
            if (data_stack->size < 2)
            {
                fputs("operator(+) failed: not enough args\n", stderr);
                break;
            }

            rplni_list_pop(data_stack, &y, &scope);
            rplni_list_pop(data_stack, &x, &scope);

            if (x.type != y.type)
            {
                fputs("error. + for different types is not implemented\n", stderr);
            }
            else if (x.type == RPLNI_UINT)
            {
                x.value._uint += y.value._uint;
                rplni_list_push(data_stack, &x, &scope);
            }
            else if (x.type == RPLNI_STR)
            {
                rplni_str_add(x.value._str, y.value._str, &scope);
                rplni_list_push(data_stack, &x, &scope);
            }

            rplni_value_clean(&x, &scope);
            rplni_value_clean(&y, &scope);

            break;
        case '.':
            // . (msg --)
            if (data_stack->size < 1)
            {
                fputs("operator(.) failed: not enough args\n", stderr);
                break;
            }

            rplni_list_pop(data_stack, &x, &scope);
            if (x.type == RPLNI_UINT)
            {
                printf("%d\n", (int)x.value._uint);
            }
            else if (x.type == RPLNI_STR)
            {
                puts(x.value._str->value);
            }
            else if (x.type == RPLNI_LIST)
            {
                putchar('[');
                for (size_t i = 0; i < x.value._list->size; ++i)
                {
                    printf("[%d]:type(%d),", (int)i, (int)x.value._list->values[i].type);
                }
                puts("]");
            }

            rplni_value_clean(&x, &scope);

            break;
        case '@':
            // pop
            // @ (list -- val list)
            if (data_stack->size < 1)
            {
                fputs("operator(@) failed: not enough args\n", stderr);
                break;
            }

            rplni_list_pop(data_stack, &x, &scope);

            if (x.type != RPLNI_LIST)
            {
                fputs("operator(@) failed: arg is not a list\n", stderr);
                break;
            }

            rplni_list_pop(x.value._list, &y, &scope);

            rplni_list_push(data_stack, &y, &scope);
            rplni_list_push(data_stack, &x, &scope);

            rplni_value_clean(&x, &scope);
            rplni_value_clean(&y, &scope);

            break;
        case '\"':
            // dup
            // " (x -- x x)
            if (data_stack->size < 1)
            {
                fputs("operator(\") failed: not enough args\n", stderr);
                break;
            }

            rplni_list_pop(data_stack, &x, &scope);

            rplni_list_push(data_stack, &x, &scope);
            rplni_list_push(data_stack, &x, &scope);

            rplni_value_clean(&x, &scope);

            break;
        case '!':
            // push
            // ! (val obj -- obj)
            if (data_stack->size < 2)
            {
                fputs("operator(!) failed: not enough args\n", stderr);
                break;
            }

            rplni_list_pop(data_stack, &y, &scope);
            rplni_list_pop(data_stack, &x, &scope);

            if (x.type != RPLNI_LIST)
            {
                fputs("operator(!) failed: arg is not a list\n", stderr);
                break;
            }

            rplni_list_push(x.value._list, &y, &scope);
            rplni_list_push(data_stack, &x, &scope);

            rplni_value_clean(&x, &scope);
            rplni_value_clean(&y, &scope);

            break;
        case '^':
            // export to global_scope
            // ^ (obj --)
            if (data_stack->size < 1)
            {
                fputs("operator(^) failed: not enough args\n", stderr);
                break;
            }

            rplni_list_pop(data_stack, &x, &scope);

            rplni_scope_export_value(&scope, &x, &global_scope);

            rplni_value_clean(&x, &scope);

            break;
        default:
            // every digit pushes integer.
            // every alphabet pushes single-character string
            if (isdigit(cmd))
            {
                x.type = RPLNI_UINT;
                x.value._uint = cmd ^ '0';
            }
            else
            {
                char buf[2] = {0, };
                buf[0] = cmd;

                x.type = RPLNI_STR;
                x.value._str = rplni_str_new(buf, &scope);
            }

            rplni_list_push(data_stack, &x, &scope);
            rplni_value_clean(&x, &scope);

            break;
        }

    }

    if (!rplni_list_unref(data_stack, &scope) || !rplni_list_unref(list_head_stack, &scope))
    {
        puts("failed unref(data_stack)");
    }

    rplni_scope_clean(&scope);
    rplni_scope_clean(&global_scope);


    return 0;
}
