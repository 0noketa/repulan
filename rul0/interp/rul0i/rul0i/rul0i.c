#include <stdio.h>
#include <ctype.h>
#include "rul0i.h"
#include "rul0i_macros.h"


int main()
{
    struct rul0i_list* data_stack = rul0i_list_new(32);
    if (!data_stack)
    {
        puts("failed to alloc list");
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
        case '+':
            if (data_stack->size < 2)
            {
                fputs("operator failed (+): not enough args\n", stderr);
                break;
            }

            rul0i_list_pop(data_stack, &y);
            rul0i_list_pop(data_stack, &x);

            if (x.type != y.type)
            {
                fputs("error. + for different types is not implemented\n", stderr);
            }
            else if (x.type == RUL0I_UINT)
            {
                x.value._uint += y.value._uint;
                rul0i_list_push(data_stack, &x);
            }
            else if (x.type == RUL0I_STR)
            {
                rul0i_str_add(x.value._str, y.value._str);
                rul0i_list_push(data_stack, &x);
            }

            rul0i_value_clean(&x, NULL);
            rul0i_value_clean(&y, NULL);

            break;
        case '.':
            if (data_stack->size < 1)
            {
                fputs("operator failed (.): not enough args\n", stderr);
                break;
            }

            rul0i_list_pop(data_stack, &x);
            if (x.type == RUL0I_UINT)
            {
                printf("%d\n", (int)x.value._uint);
            }
            else if (x.type == RUL0I_STR)
            {
                puts(x.value._str->value);
            }

            rul0i_value_clean(&x, NULL);

            break;
        default:
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
                x.value._str = rul0i_str_new(buf);
            }

            rul0i_list_push(data_stack, &x);
            rul0i_value_clean(&x, NULL);

            break;
        }

    }

    if (!rul0i_list_unref(data_stack, NULL))
    {
        puts("failed unref(data_stack)");
    }


    return 0;
}
