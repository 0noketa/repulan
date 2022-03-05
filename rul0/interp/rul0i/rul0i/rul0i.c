#include <stdio.h>
#include <ctype.h>
#include "rul0i.h"
#include "rul0i_macros.h"


// interpreter for partial debugging
int main()
{
    int initialization_error = 0;
    struct rul0i_scope global_scope;
    struct rul0i_scope scope;
    if (!rul0i_scope_init(&global_scope)) initialization_error = 1;
    if (!initialization_error && !rul0i_scope_init(&scope))
    {
        rul0i_scope_clean(&global_scope);

        initialization_error = 1;
    }

    if (initialization_error)
    {
        puts("failed to init scope");
        return 1;
    }

    struct rul0i_list* data_stack = rul0i_list_new(32, &scope);
    struct rul0i_list* list_head_stack = rul0i_list_new(32, &scope);
    if (!data_stack || !list_head_stack)
    {
        if (data_stack) rul0i_list_unref(data_stack, &scope);
        if (list_head_stack) rul0i_list_unref(data_stack, &scope);

        puts("failed to alloc lists");
        return 1;
    }

    RUL0I_DEF(x);
    RUL0I_DEF(y);

    while (!feof(stdin))
    {
        int cmd = getchar();

        if (strchr(" \t\n\r", cmd)) continue;

        switch (cmd)
        {
        case '[':
            // [ (--)
            x.type = RUL0I_UINT;
            x.value._uint = data_stack->size;
            rul0i_list_push(list_head_stack, &x, &scope);
            break;
        case ']':
            // ] (-- list)
            if (list_head_stack->size == 0)
            {
                fputs("operator(]) failed: ([) was not used\n", stderr);
                break;
            }
            rul0i_list_pop(list_head_stack, &x, &scope);
            y.type = RUL0I_LIST;
            {
                size_t size = data_stack->size - x.value._uint;
                y.value._list = rul0i_list_new_with_captured(size, data_stack, &scope);
                rul0i_list_push(data_stack, &y, &scope);
            }
            break;
        case '+':
            // + (x y -- x+y)
            if (data_stack->size < 2)
            {
                fputs("operator(+) failed: not enough args\n", stderr);
                break;
            }

            rul0i_list_pop(data_stack, &y, &scope);
            rul0i_list_pop(data_stack, &x, &scope);

            if (x.type != y.type)
            {
                fputs("error. + for different types is not implemented\n", stderr);
            }
            else if (x.type == RUL0I_UINT)
            {
                x.value._uint += y.value._uint;
                rul0i_list_push(data_stack, &x, &scope);
            }
            else if (x.type == RUL0I_STR)
            {
                rul0i_str_add(x.value._str, y.value._str, &scope);
                rul0i_list_push(data_stack, &x, &scope);
            }

            rul0i_value_clean(&x, &scope);
            rul0i_value_clean(&y, &scope);

            break;
        case '.':
            // . (msg --)
            if (data_stack->size < 1)
            {
                fputs("operator(.) failed: not enough args\n", stderr);
                break;
            }

            rul0i_list_pop(data_stack, &x, &scope);
            if (x.type == RUL0I_UINT)
            {
                printf("%d\n", (int)x.value._uint);
            }
            else if (x.type == RUL0I_STR)
            {
                puts(x.value._str->value);
            }
            else if (x.type == RUL0I_LIST)
            {
                putchar('[');
                for (size_t i = 0; i < x.value._list->size; ++i)
                {
                    printf("[%d]:type(%d),", (int)i, (int)x.value._list->values[i].type);
                }
                puts("]");
            }

            rul0i_value_clean(&x, &scope);

            break;
        case '@':
            // pop
            // @ (list -- val list)
            if (data_stack->size < 1)
            {
                fputs("operator(@) failed: not enough args\n", stderr);
                break;
            }

            rul0i_list_pop(data_stack, &x, &scope);

            if (x.type != RUL0I_LIST)
            {
                fputs("operator(@) failed: arg is not a list\n", stderr);
                break;
            }

            rul0i_list_pop(x.value._list, &y, &scope);

            rul0i_list_push(data_stack, &y, &scope);
            rul0i_list_push(data_stack, &x, &scope);

            rul0i_value_clean(&x, &scope);
            rul0i_value_clean(&y, &scope);

            break;
        case '\"':
            // dup
            // " (x -- x x)
            if (data_stack->size < 1)
            {
                fputs("operator(\") failed: not enough args\n", stderr);
                break;
            }

            rul0i_list_pop(data_stack, &x, &scope);

            rul0i_list_push(data_stack, &x, &scope);
            rul0i_list_push(data_stack, &x, &scope);

            rul0i_value_clean(&x, &scope);

            break;
        case '!':
            // push
            // ! (val obj -- obj)
            if (data_stack->size < 2)
            {
                fputs("operator(!) failed: not enough args\n", stderr);
                break;
            }

            rul0i_list_pop(data_stack, &y, &scope);
            rul0i_list_pop(data_stack, &x, &scope);

            if (x.type != RUL0I_LIST)
            {
                fputs("operator(!) failed: arg is not a list\n", stderr);
                break;
            }

            rul0i_list_push(x.value._list, &y, &scope);
            rul0i_list_push(data_stack, &x, &scope);

            rul0i_value_clean(&x, &scope);
            rul0i_value_clean(&y, &scope);

            break;
        case '^':
            // export to global_scope
            // ^ (obj --)
            if (data_stack->size < 1)
            {
                fputs("operator(^) failed: not enough args\n", stderr);
                break;
            }

            rul0i_list_pop(data_stack, &x, &scope);

            rul0i_scope_export_value(&scope, &x, &global_scope);

            rul0i_value_clean(&x, &scope);

            break;
        default:
            // every digit pushes integer.
            // every alphabet pushes single-character string
            if (isdigit(cmd))
            {
                x.type = RUL0I_UINT;
                x.value._uint = cmd ^ '0';
            }
            else
            {
                char buf[2] = {0, };
                buf[0] = cmd;

                x.type = RUL0I_STR;
                x.value._str = rul0i_str_new(buf, &scope);
            }

            rul0i_list_push(data_stack, &x, &scope);
            rul0i_value_clean(&x, &scope);

            break;
        }

    }

    if (!rul0i_list_unref(data_stack, &scope) || !rul0i_list_unref(list_head_stack, &scope))
    {
        puts("failed unref(data_stack)");
    }

    rul0i_scope_clean(&scope);
    rul0i_scope_clean(&global_scope);


    return 0;
}
