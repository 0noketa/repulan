#include "rplni.h"
#include "rplni_macro.h"


#ifndef NDEBUG
static struct rplni_ptrlist* all_objs;
#endif


int rplni_value_init(struct rplni_value* value)
{
    if (value == NULL) return 0;

    value->type = RPLNI_UINT;
    value->value._uint = 0;

    return 1;
}
int rplni_value_init_with_uint(struct rplni_value* value, uintptr_t uint_value)
{
    if (value == NULL) return 0;

    value->type = RPLNI_UINT;
    value->value._uint = uint_value;

    return 1;
}
int rplni_value_init_with_cstr(struct rplni_value* value, char* str_value, struct rplni_scope* scope)
{
    if (value == NULL || str_value == NULL) return 0;

    struct rplni_str* str = rplni_str_new(str_value, scope);
    if (str == NULL) return 0;

    value->type = RPLNI_STR;
    value->value._str = str;

    return 1;
}
int rplni_value_init_with_empty_func(struct rplni_value* value, enum rplni_func_type type, struct rplni_scope* scope)
{
    if (value == NULL) return 0;

    struct rplni_func *func = rplni_func_new(type, scope);
    if (func == NULL) return 0;

    value->type = RPLNI_FUNC;
    value->value._func = func;

    return 1;
}

int rplni_value_clean(struct rplni_value *value, struct rplni_scope* scope)
{
    if (value == NULL) return 0;

    switch (value->type)
    {
        case RPLNI_UINT:
            return 1;
        case RPLNI_STR:
            if (!rplni_str_unref(value->value._str, scope)) return 0;
            break;
        case RPLNI_LIST:
            if (!rplni_list_unref(value->value._list, scope)) return 0;
            break;
        case RPLNI_FUNC:
            if (!rplni_func_unref(value->value._func, scope)) return 0;
            break;
        case RPLNI_CLOSURE:
        case RPLNI_BUILTIN:
        default:
            return 0;
    }

    value->type = RPLNI_UINT;
    value->value._uint = 0;

    return 1;
}
int rplni_value_ref(struct rplni_value* value)
{
    if (value == NULL) return 0;

    switch (value->type)
    {
    case RPLNI_UINT:
        return 1;
    case RPLNI_STR:
        if (!rplni_str_ref(value->value._str)) return 0;
        break;
    case RPLNI_LIST:
        if (!rplni_list_ref(value->value._list)) return 0;
        break;
    case RPLNI_FUNC:
        if (!rplni_func_ref(value->value._func)) return 0;
        break;
    case RPLNI_CLOSURE:
    case RPLNI_BUILTIN:
    default:
        return 0;
    }

    return 1;
}
int rplni_value_eq(struct rplni_value* value, struct rplni_value* value2)
{
    if (value == NULL || value2 == NULL || value->type != value2->type) return 0;

    return value->type == RPLNI_STR ? value->value._str == value2->value._str
        : value->type == RPLNI_LIST ? value->value._list == value2->value._list
        : value->type == RPLNI_FUNC ? value->value._func == value2->value._func
        : value->value._uint == value2->value._uint;
}
int rplni_value_del(struct rplni_value* value, struct rplni_scope *scope)
{
    if (value == NULL) return 0;
    if (!rplni_scope_has_value(scope, value))
    {
        return rplni_value_clean(value, scope);
    }

    switch (value->type)
    {
    case RPLNI_UINT:
        return 1;
    case RPLNI_STR:
        if (!rplni_str_del(value->value._str, scope)) return 0;
        break;
    case RPLNI_LIST:
        if (!rplni_list_del(value->value._list, scope)) return 0;
        break;
    case RPLNI_FUNC:
        if (!rplni_func_del(value->value._func,scope)) return 0;
        break;
    case RPLNI_CLOSURE:
    case RPLNI_BUILTIN:
    default:
        return 0;
    }

    value->type = RPLNI_UINT;
    value->value._uint = 0;

    return 1;
}
int rplni_scope_export_value(struct rplni_scope* scope, struct rplni_value* value, struct rplni_scope* scope2)
{
    if (value == NULL || scope2 == NULL) return 0;

    switch (value->type)
    {
    case RPLNI_UINT:
        return 1;
    case RPLNI_STR:
        if (!rplni_scope_export_str(scope, value->value._str, scope2)) return 0;
        break;
    case RPLNI_LIST:
        if (!rplni_scope_export_list(scope, value->value._list, scope2)) return 0;
        break;
    case RPLNI_FUNC:
        if (!rplni_scope_export_func(scope, value->value._func, scope2)) return 0;
        break;
    case RPLNI_CLOSURE:
    case RPLNI_BUILTIN:
    default:
        return 0;
    }

    return 1;
}
int rplni_value_owner(struct rplni_value* value, struct rplni_scope** out_owner)
{
    if (value == NULL || out_owner == NULL) return 0;
    if (value->type == RPLNI_UINT) return 0;

    *out_owner = value->type == RPLNI_STR ? value->value._str->owner
        : value->type == RPLNI_LIST ? value->value._list->owner
        : value->type == RPLNI_FUNC ? value->value._func->owner
        : NULL;

    return 1;
}


int rplni_values_init(size_t size, struct rplni_value* values)
{
    if (values == NULL) return 0;

    for (size_t i = 0; i < size; ++i)
    {
        rplni_value_init(values + i);
    }

    return 1;
}
size_t rplni_values_object_index(size_t size, struct rplni_value* values, struct rplni_value* value)
{
    if (values == NULL) return 0;
    if (value == NULL) return size;

    size_t idx = 0;
    for (; idx < size && values[idx].type != RPLNI_UINT && !rplni_value_eq(values + idx, value); ++idx);

    return idx;
}
int rplni_values_has_object(size_t size, struct rplni_value* values, struct rplni_value* value)
{
    if (values == NULL || value == NULL || value->type == RPLNI_UINT) return 0;

    size_t idx = rplni_values_object_index(size, values, value);
    return idx < size;
}
int rplni_values_add_object(
        size_t size, struct rplni_value* values,
        struct rplni_value* value,
        struct rplni_scope* scope,
        size_t *out_size, struct rplni_value* out_values)
{
    if (values == NULL) return 0;

    size_t idx = rplni_values_object_index(size, values, value);
    if (idx >= size)
    {
        size_t size2 = size + RPLNI_LIST_BLOCK_SIZE;
        struct rplni_value* values2 = rplni_scope_realloc(scope, values, size2 * sizeof(struct rplni_value));

        if (values2 == NULL) return 0;

        rplni_values_init(size2 - size, values + size);

        values = values2;
        size = size2;
    }

    values[idx] = *value;

    *out_size = size;
    *out_values = *values;

    return 1;
}
int rplni_values_remove_object(size_t size, struct rplni_value* values, struct rplni_value* value)
{
    if (values == NULL) return 0;

    size_t idx = rplni_values_object_index(size, values, value);
    if (idx >= size) return 0;

    rplni_value_init(values + idx);

    return 1;
}


int rplni_named_init(struct rplni_named *named, char *name, struct rplni_scope* scope)
{
    if (named == NULL || name == NULL) return 0;

    named->name = malloc(strlen(name) + 1);

    rplni_value_init(&(named->value));

    if (named->name != NULL)
    {
        strcpy(named->name, name);

        return 1;
    }

    return 0;
}
int rplni_named_clean(struct rplni_named *named, struct rplni_scope *scope)
{
    if (named == NULL) return 0;

    rplni_value_clean(&(named->value), scope);

    free(named->name);
    named->name = NULL;

    return 1;
}

int rplni_cmd_init(struct rplni_cmd* cmd, enum rplni_op_type op, struct rplni_value *arg, struct rplni_scope* scope)
{
    if (cmd == NULL) return 0;

    cmd->op = RPLNI_OP_NOP;
    rplni_value_init(&cmd->arg);

    if (arg != NULL && !rplni_value_ref(arg)) return 0;

    cmd->op = op;
    if (arg != NULL) cmd->arg = *arg;

    return 1;
}
int rplni_cmd_clean(struct rplni_cmd* cmd, struct rplni_scope* scope)
{
    if (cmd == NULL) return 0;
    if (!rplni_value_clean(&(cmd->arg), scope)) return 0;

    cmd->op = RPLNI_OP_NOP;

    return 1;
}
int rplni_cmd_del(struct rplni_cmd* cmd, struct rplni_scope* scope)
{
    if (cmd == NULL) return 0;
    if (!rplni_value_del(&(cmd->arg), scope)) return 0;

    cmd->op = RPLNI_OP_NOP;
    return 1;
}
int rplni_prog_init(struct rplni_prog* prog, struct rplni_scope* scope)
{
    if (prog == NULL) return 0;

    prog->cap = RPLNI_LIST_CEILED_SIZE(0);
    prog->size = 0;
    prog->code = rplni_scope_malloc(scope, prog->cap * sizeof(struct rplni_cmd));

    if (prog->code == NULL)
    {
        prog->cap = 0;

        return 0;
    }

    return 1;
}
int rplni_prog_add(struct rplni_prog* prog, struct rplni_cmd* cmd, struct rplni_scope* scope)
{
    if (prog->size >= prog->cap)
    {
        size_t cap = prog->cap + RPLNI_LIST_BLOCK_SIZE;
        struct rplni_cmd *code = rplni_scope_realloc(scope, prog->code, cap * sizeof(struct rplni_cmd));

        if (code == NULL) return 0;

        prog->cap = cap;
        prog->code = code;
    }

    prog->code[prog->size++] = *cmd;

    return 1;
}
int rplni_prog_clean(struct rplni_prog* prog, struct rplni_scope* owner, struct rplni_scope* scope)
{
    if (prog == NULL || scope == NULL) return 0;

    for (size_t i = 0; i < prog->size; ++i)
    {
        rplni_cmd_clean(prog->code + i, scope);
    }

    prog->cap = 0;
    prog->size = 0;
    rplni_scope_free(owner, prog->code);
    prog->code = NULL;
    return 1;
}
int rplni_prog_del(struct rplni_prog* prog, struct rplni_scope* owner, struct rplni_scope* scope)
{
    if (prog == NULL || scope == NULL) return 0;

    for (size_t i = 0; i < prog->size; ++i)
    {
        rplni_cmd_del(prog->code + i, scope);
    }

    prog->cap = 0;
    prog->size = 0;
    rplni_scope_free(owner, prog->code);
    prog->code = NULL;
    return 1;
}
int rplni_scope_export_prog(struct rplni_scope* scope, struct rplni_prog* prog, struct rplni_scope* scope2)
{
    if (scope == NULL || prog == NULL || scope2 == NULL) return 0;
    if (!rplni_scope_has(scope, prog->code)) return 0;

    for (size_t i = 0; i < prog->size; ++i)
    {
        rplni_scope_export_value(scope, &(prog->code[i].arg), scope2);
    }
    rplni_scope_add(scope2, prog->code);
    rplni_scope_remove(scope, prog->code);

    return 1;
}
int rplni_prog_run(struct rplni_prog* prog, struct rplni_state* state)
{
    if (prog == NULL || state == NULL) return 0;

    struct rplni_scope* scope;
    if (!rplni_state_current_scope(state, &scope)) return 0;

    struct rplni_list* data_stack = state->data_stack;
    struct rplni_list* list_head_stack = state->list_head_stack;

    RPLNI_DEF(x);
    RPLNI_DEF(y);

    for (size_t i = 0; i < prog->size; ++i)
    {
        struct rplni_cmd* cmd = prog->code + i;
        switch (cmd->op)
        {
        case RPLNI_OP_BEGIN_ARGS:
            rplni_list_pop(data_stack, &x);
            if (x.type != RPLNI_FUNC && x.type != RPLNI_LIST)
            {
                fputs("instruction [(] failed: argument should be a function or list\n", stderr);
                break;
            }
            rplni_list_push(state->arg0_stack, &x);

            y.type = RPLNI_UINT;
            y.value._uint = data_stack->size;
            rplni_list_push(state->arg0_stack, &y);

            rplni_value_clean(&x, NULL);
            rplni_value_clean(&y, NULL);
            break;
        case RPLNI_OP_END_ARGS:
            rplni_list_pop(state->arg0_stack, &x);
            rplni_list_pop(state->arg0_stack, &y);

            if (y.type == RPLNI_LIST)
            {
                struct rplni_list* list = y.value._list;
                size_t argss_size = data_stack->size - x.value._uint;

                RPLNI_DEF(argss);
                argss.type = RPLNI_LIST;
                argss.value._list = rplni_list_new_with_captured(argss_size, data_stack, scope);

                for (size_t i = 0; i < argss.value._list->size; ++i)
                {
                    struct rplni_value* v = argss.value._list->values + i;
                    if (v->type != RPLNI_UINT)
                    {
                        fputs("instruction [)] (for list) failed: captured arguments should be uint\n", stderr);
                        continue;
                    }

                    rplni_list_push(data_stack, y.value._list->values + v->value._uint);
                }

                rplni_value_clean(&argss, NULL);
            }
            else if (y.type == RPLNI_FUNC)
            {
                struct rplni_func* f = y.value._func;
                size_t argc = rplni_ptrlist_len(f->params);
                size_t argss_size = data_stack->size - x.value._uint;

                if (argc == 0)
                {
                    rplni_func_run(f, state);
                }
                else
                {
                    RPLNI_DEF(argss);
                    argss.type = RPLNI_LIST;
                    argss.value._list = rplni_list_new_with_captured(argss_size, data_stack, scope);

                    for (size_t i = 0; i < argss_size; i += argc)
                    {
                        for (size_t j = 0; j < argc; ++j)
                        {
                            printf("idx:%d\n", (int)(i + j));
                            struct rplni_value *val = argss.value._list->values + i + j;
                            rplni_list_push(data_stack, val);
                        }

                        rplni_func_run(f, state);
                    }

                    rplni_value_clean(&argss, NULL);
                }
            }

            rplni_value_clean(&x, NULL);
            rplni_value_clean(&y, NULL);
            break;
        case RPLNI_OP_PUSH:
            rplni_list_push(data_stack, &(cmd->arg));
            break;
        case RPLNI_OP_DROP:
            if (data_stack->size < 1)
            {
                fputs("operator(drop) failed: not enough args\n", stderr);
                break;
            }

            rplni_list_pop(data_stack, &x);
            rplni_value_clean(&x, NULL);
            break;
        case RPLNI_OP_DUP:
            // dup (x -- x x)
            if (data_stack->size < 1)
            {
                fputs("operator(dup) failed: not enough args\n", stderr);
                break;
            }

            rplni_list_pop(data_stack, &x);

            rplni_list_push(data_stack, &x);
            rplni_list_push(data_stack, &x);

            rplni_value_clean(&x, NULL);
            break;
        case RPLNI_OP_SWAP:
            if (data_stack->size < 2)
            {
                fputs("operator(swap) failed: not enough args\n", stderr);
                break;
            }

            rplni_list_pop(data_stack, &y);
            rplni_list_pop(data_stack, &x);
            rplni_list_push(data_stack, &y);
            rplni_list_push(data_stack, &x);
            rplni_value_clean(&x, NULL);
            rplni_value_clean(&y, NULL);
            break;
        case RPLNI_OP_LOAD:
            if (cmd->arg.type == RPLNI_UINT)
            {
                if (!rplni_scope_load_var_by_index(scope, cmd->arg.value._uint, &x)) break;
            }
            else if (cmd->arg.type == RPLNI_STR)
            {
                if (!rplni_state_load_var(state, cmd->arg.value._str->value, &x)) break;
            }
            else
            {
                fputs("operator(load) failed: argument should be any of int and str\n", stderr);
                break;
            }

            rplni_list_push(data_stack, &x);
            rplni_value_clean(&x, NULL);
            break;
        case RPLNI_OP_STORE:
            rplni_list_pop(data_stack, &x);

            if (cmd->arg.type == RPLNI_UINT)
            {
                if (!rplni_scope_store_var_by_index(scope, cmd->arg.value._uint, &x)) break;
            }
            else if (cmd->arg.type == RPLNI_STR)
            {
                if (!rplni_state_store_var(state, cmd->arg.value._str->value, &x)) break;
            }
            else
            {
                fputs("operator(store) failed: argument should be any of int and str\n", stderr);
                break;
            }
            rplni_value_clean(&x, NULL);
            break;
        case RPLNI_OP_BEGIN_LIST:
            // [ (--)
            x.type = RPLNI_UINT;
            x.value._uint = data_stack->size;
            rplni_list_push(list_head_stack, &x);
            rplni_value_clean(&x, NULL);
            break;
        case RPLNI_OP_END_LIST:
            // ] (-- list)
            if (list_head_stack->size == 0)
            {
                fputs("operator(]) failed: ([) was not used\n", stderr);
                break;
            }
            rplni_list_pop(list_head_stack, &x);
            y.type = RPLNI_LIST;
            {
                size_t size = data_stack->size - x.value._uint;
                y.value._list = rplni_list_new_with_captured(size, data_stack, scope);
                rplni_list_push(data_stack, &y);
                rplni_value_clean(&y, NULL);
            }
            rplni_value_clean(&x, NULL);
            break;
        case RPLNI_OP_ADD:
            // + (x y -- x+y)
            if (data_stack->size < 2)
            {
                fputs("operator(+) failed: not enough args\n", stderr);
                break;
            }

            rplni_list_pop(data_stack, &y);
            rplni_list_pop(data_stack, &x);

            if (x.type != y.type)
            {
                fputs("error. + for different types is not implemented\n", stderr);
            }
            else if (x.type == RPLNI_UINT)
            {
                x.value._uint += y.value._uint;
                rplni_list_push(data_stack, &x);
            }
            else if (x.type == RPLNI_STR)
            {
                RPLNI_DEF_STR(tmp, x.value._str->value, scope);

                rplni_str_add(tmp.value._str, y.value._str);
                rplni_list_push(data_stack, &tmp);

                fprintf(stderr, "new str %p @ %p (%p @ %p  +  %p @ %p)\n",
                    tmp.value._str->value, scope,
                    x.value._str->value, x.value._str->owner,
                    y.value._str->value, y.value._str->owner);

                rplni_value_clean(&tmp, NULL);
            }
            rplni_value_clean(&x, NULL);
            rplni_value_clean(&y, NULL);
            break;
        case RPLNI_OP_MUL:
            // * (x y -- x*y)
            if (data_stack->size < 2)
            {
                fputs("operator(*) failed: not enough args\n", stderr);
                break;
            }

            rplni_list_pop(data_stack, &y);
            rplni_list_pop(data_stack, &x);

            if (x.type == RPLNI_UINT && y.type == RPLNI_UINT)
            {
                x.value._uint *= y.value._uint;
                rplni_list_push(data_stack, &x);
            }
            else
            {
                fputs("error. * for non-integer values is not implemented\n", stderr);
            }

            rplni_value_clean(&x, NULL);
            rplni_value_clean(&y, NULL);
            break;
        case RPLNI_OP_RANGE:
            if (data_stack->size < 2)
            {
                fputs("range requires 2 args\n", stderr);
                break;
            }

            rplni_list_pop(data_stack, &y);
            rplni_list_pop(data_stack, &x);

            if (x.type != RPLNI_UINT || y.type != RPLNI_UINT)
            {
                fputs("range for non-integer args is not implemented", stderr);
                break;
            }

            for (size_t i = x.value._uint; i < y.value._uint; ++i)
            {
                RPLNI_DEF_UINT(tmp, i);

                rplni_list_push(data_stack, &tmp);

                rplni_value_clean(&tmp, NULL);
            }

            rplni_value_clean(&x, NULL);
            rplni_value_clean(&y, NULL);
            break;
        case RPLNI_OP_CALL:
            // call (unary_func -- )
            fputs("try call\n", stderr);
            if (data_stack->size < 1)
            {
                fputs("operator(call) failed: not enough args\n", stderr);
                break;
            }

            rplni_list_pop(data_stack, &x);
            if (x.type != RPLNI_FUNC)
            {
                fputs("operator(call) failed: arg is not a func\n", stderr);
            }

            rplni_func_run(x.value._func, state);

            rplni_value_clean(&x, NULL);
            break;
        case RPLNI_OP_PRINT:
            // . (msg --)
            if (data_stack->size < 1)
            {
                fputs("operator(__print) failed: not enough args\n", stderr);
                break;
            }

            rplni_list_pop(data_stack, &x);
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
                    struct rplni_value* member = x.value._list->values + i;
                    printf("%d:", (int)i);
                    if (member->type == RPLNI_UINT)
                    {
                        printf("UINT(%u),", (int)member->value._uint);
                    }
                    else if (member->type == RPLNI_STR)
                    {
                        printf("STR(%s),", member->value._str->value);
                    }
                    else
                    {
                        printf("TYPE(%d)(...),", (int)member->type);
                    }
                }
                puts("]");
            }
            rplni_value_clean(&x, NULL);
            break;
        case RPLNI_OP_LEN:
            // __len (list -- length)
            if (data_stack->size < 1)
            {
                fputs("operator(__len) failed: not enough args\n", stderr);
                break;
            }

            rplni_list_pop(data_stack, &x);

            if (x.type != RPLNI_LIST)
            {
                fputs("operator(__len) failed: arg is not a list\n", stderr);
                break;
            }

            rplni_value_init_with_uint(&y, (uintptr_t)x.value._list->size);
            rplni_list_push(data_stack, &y);

            rplni_value_clean(&x, NULL);
            rplni_value_clean(&y, NULL);
            break;
        case RPLNI_OP_LIST_POP:
            // pop
            // @ (list -- val list)
            if (data_stack->size < 1)
            {
                fputs("operator(@) failed: not enough args\n", stderr);
                break;
            }

            rplni_list_pop(data_stack, &x);

            if (x.type != RPLNI_LIST)
            {
                fputs("operator(@) failed: arg is not a list\n", stderr);
                break;
            }

            rplni_list_pop(x.value._list, &y);

            rplni_list_push(data_stack, &y);
            rplni_list_push(data_stack, &x);

            rplni_value_clean(&x, NULL);
            rplni_value_clean(&y, NULL);
            break;
        case RPLNI_OP_LIST_PUSH:
            // push
            // ! (val obj -- obj)
            if (data_stack->size < 2)
            {
                fputs("instruction(list_push) failed: not enough args\n", stderr);
                break;
            }

            rplni_list_pop(data_stack, &y);
            rplni_list_pop(data_stack, &x);

            if (x.type != RPLNI_LIST)
            {
                fputs("instruction(list_push) failed: arg is not a list\n", stderr);
                break;
            }

            rplni_state_gather_values(state, &y, &x);

            rplni_list_push(x.value._list, &y);
            rplni_list_push(data_stack, &x);

            rplni_value_clean(&x, NULL);
            rplni_value_clean(&y, NULL);
            break;
        case RPLNI_OP_SPREAD:
            // __spread (list -- list(0 len(list):))
            if (data_stack->size < 1)
            {
                fputs("operator(__spreat) failed: not enough args\n", stderr);
                break;
            }

            rplni_list_pop(data_stack, &x);

            if (x.type != RPLNI_LIST)
            {
                fputs("operator(__spreat) failed: arg is not a list\n", stderr);
                break;
            }

            for (size_t i = 0; i < x.value._list->size; ++i)
            {
                struct rplni_value* v = x.value._list->values + i;

                rplni_list_push(data_stack, v);
            }

            rplni_value_clean(&x, NULL);
            rplni_value_clean(&y, NULL);
            break;
        case RPLNI_OP_EXPORT:
            // export to outer_scope
            // ^ (obj --)
            if (data_stack->size < 1)
            {
                fputs("instruction(export) failed: not enough args\n", stderr);
                break;
            }

            rplni_list_pop(data_stack, &x);

            {
                struct rplni_scope* outer_scope;
                if (!rplni_state_outer_scope(state, &outer_scope))
                {
                    fputs("instruction(export) failed: any error\n", stderr);
                    break;
                }

                rplni_scope_export_value(scope, &x, outer_scope);
            }

            rplni_value_clean(&x, NULL);
            break;
        default:
            fprintf(stderr, "unknown op(%d)\n", (int)cmd->op);
            return 0;
        }
    }

    return 1;
}


int rplni_scope_init(struct rplni_scope *scope)
{
    if (scope == NULL) return 0;

    scope->objs = rplni_ptrlist_new();
    scope->strs = rplni_ptrlist_new();
    scope->lists = rplni_ptrlist_new();
    scope->funcs = rplni_ptrlist_new();
    scope->deallocation_history = rplni_ptrlist_new();

    scope->cap = 1;
    scope->size = 0;
    scope->vars = rplni_scope_malloc(scope, sizeof(struct rplni_named));

    return scope->vars != NULL;
}
int rplni_scope_clean(struct rplni_scope *scope)
{
    if (scope == NULL) return 0;

    fprintf(stderr, "clean scope %p\n", scope);

    for (size_t i = 0; i < scope->size; ++i)
    {
        rplni_named_clean(scope->vars + i, scope);
    }
    rplni_scope_free(scope, scope->vars);
    scope->size = 0;
    scope->vars = NULL;

    for (size_t i = 0; i < rplni_ptrlist_len(scope->lists); ++i)
    {
        struct rplni_list* obj = scope->lists->values.list[i];

        fprintf(stderr, "del list%p\n", obj);
        rplni_list_del(obj, scope);
    }
    rplni_ptrlist_del(scope->lists);
    scope->lists = NULL;

    for (size_t i = 0; i < rplni_ptrlist_len(scope->strs); ++i)
    {
        struct rplni_str* obj = scope->strs->values.str[i];

        fprintf(stderr, "del str%p\n", obj);
        rplni_str_del(obj, scope);
    }
    rplni_ptrlist_del(scope->strs);
    scope->strs = NULL;

    for (size_t i = 0; i < rplni_ptrlist_len(scope->funcs); ++i)
    {
        struct rplni_func* obj = scope->funcs->values.func[i];

        fprintf(stderr, "del func%p\n", obj);
        rplni_func_del(obj, scope);
    }
    rplni_ptrlist_del(scope->funcs);
    scope->funcs = NULL;

    for (size_t i = 0; i < rplni_ptrlist_len(scope->objs); ++i)
    {
        void *p = scope->objs->values.any[i];

        fprintf(stderr, "del any %p\n", p);
        rplni_scope_free(scope, p);
    }
    rplni_ptrlist_del(scope->objs);
    scope->objs = NULL;

    rplni_ptrlist_del(scope->deallocation_history);
    scope->deallocation_history = NULL;

    fputs("end clean scope\n", stderr);

    return 1;
}
void *rplni_scope_malloc(struct rplni_scope* scope, size_t size)
{
    void* p = malloc(size);

#ifndef NDEBUG
    if (all_objs == NULL) all_objs = rplni_ptrlist_new();
    rplni_ptrlist_add(all_objs, p);
#endif

    fprintf(stderr, "+%p @ %p (size: %d)\n", p, scope, (int)size);

    if (scope == NULL) return p;

    if (rplni_ptrlist_add(scope->objs, p)) return p;

    free(p);

    return NULL;
}
void* rplni_scope_realloc(struct rplni_scope* scope, void *p, size_t size)
{
#ifndef NDEBUG
    if (all_objs == NULL) all_objs = rplni_ptrlist_new();
    assert(rplni_ptrlist_has(all_objs, p));
    rplni_ptrlist_remove(all_objs, p);
#endif

    fprintf(stderr, "-%p @ %p (new_size: %d)\n", p, scope, (int)size);
    if (p == NULL) return NULL;
    if (!rplni_scope_has(scope, p)) return NULL;

    void* q;
    if (size == 0)
    {
        q = malloc(size);
        if (q == NULL) return NULL;
        free(p);
    }
    else
    {
        q = realloc(p, size);
        if (q == NULL) return NULL;
    }

    fprintf(stderr, "+%p @ %p (reallocated)\n", q, scope);

#ifndef NDEBUG
    rplni_ptrlist_add(all_objs, q);
#endif

    if (scope == NULL) return q;

    if (scope != NULL
        && rplni_ptrlist_remove(scope->objs, p)
        && rplni_ptrlist_add(scope->objs, q))
    {
        return q;
    }

    free(q);
    return NULL;
}
int rplni_scope_free(struct rplni_scope* scope, void* p)
{
#ifndef NDEBUG
    if (all_objs == NULL) all_objs = rplni_ptrlist_new();
    assert(rplni_ptrlist_has(all_objs, p));
    rplni_ptrlist_remove(all_objs, p);
#endif

    if (p == NULL) return 0;

    if (scope == NULL || rplni_ptrlist_remove(scope->objs, p))
    {
        fprintf(stderr, "-%p @ %p\n", p, scope);
        free(p);

        return 1;
    }

    fprintf(stderr, "failed -%p @ %p\n", p, scope);

    return 0;
}
int rplni_scope_add_var(struct rplni_scope* scope, char* name)
{
    if (scope == NULL || name == NULL) return 0;
    if (rplni_scope_has_var(scope, name)) return 0;

    if (scope->size >= scope->cap)
    {
        size_t cap = scope->cap + RPLNI_LIST_BLOCK_SIZE;
        struct rplni_named* vars = rplni_scope_realloc(scope, scope->vars, cap * sizeof(struct rplni_named));

        if (vars == NULL) return 0;

        scope->cap = cap;
        scope->vars = vars;
    }

    if (!rplni_named_init(scope->vars + scope->size, name, scope)) return 0;

    scope->size++;
    return 1;
}
size_t rplni_scope_var_index(struct rplni_scope* scope, char* name)
{
    if (scope == NULL) return 0;
    if (name == NULL) return scope->size;

    size_t i = 0;
    for (; i < scope->size; ++i)
    {
        if (!strcmp(scope->vars[i].name, name)) return i;
    }

    return i;
}
int rplni_scope_has_var(struct rplni_scope* scope, char* name)
{
    return rplni_scope_var_index(scope, name) < scope->size;
}
int rplni_scope_load_var_by_index(struct rplni_scope* scope, size_t idx, struct rplni_value* out_value)
{
    if (scope == NULL || out_value == NULL) return 0;
    if (idx >= scope->size) return 0;

    struct rplni_named* v = scope->vars + idx;

    if (!rplni_value_ref(&(v->value))) return 0;

    *out_value = v->value;
    return 1;
}
int rplni_scope_load_var(struct rplni_scope* scope, char* name, struct rplni_value* out_value)
{
    if (scope == NULL || name == NULL || out_value == NULL) return 0;
    size_t idx = rplni_scope_var_index(scope, name);

    return rplni_scope_load_var_by_index(scope, idx, out_value);
}
int rplni_scope_store_var_by_index(struct rplni_scope* scope, size_t idx, struct rplni_value* value)
{
    if (scope == NULL || value == NULL) return 0;
    if (idx >= scope->size) return 0;

    struct rplni_named* v = scope->vars + idx;

    if (!rplni_value_ref(value)) return 0;

    v->value = *value;
    return 1;
}
int rplni_scope_store_var(struct rplni_scope* scope, char* name, struct rplni_value* value)
{
    if (scope == NULL || name == NULL || value == NULL) return 0;
    size_t idx = rplni_scope_var_index(scope, name);

    return rplni_scope_store_var_by_index(scope, idx, value);
}
int rplni_scope_add(struct rplni_scope* scope, void* p)
{
    return scope != NULL && rplni_ptrlist_add(scope->objs, p);
}
int rplni_scope_has(struct rplni_scope* scope, void* p)
{
    return scope != NULL && rplni_ptrlist_has(scope->objs, p);
}
int rplni_scope_remove(struct rplni_scope* scope, void* p)
{
    return scope != NULL && rplni_ptrlist_remove(scope->objs, p);
}
int rplni_scope_has_value(struct rplni_scope* scope, struct rplni_value* p)
{
    if (scope == NULL || p == NULL) return 0;

    switch (p->type)
    {
    case RPLNI_UINT:
        return 1;
    case RPLNI_STR:
        return rplni_scope_has_str(scope, p->value._str);
    case RPLNI_LIST:
        return rplni_scope_has_list(scope, p->value._list);
    case RPLNI_FUNC:
        return rplni_scope_has_func(scope, p->value._func);
    case RPLNI_CLOSURE:
    case RPLNI_BUILTIN:
    default:
        return 0;
    }
}
int rplni_scope_add_value(struct rplni_scope* scope, struct rplni_value* p)
{
    if (scope == NULL || p == NULL) return 0;

    switch (p->type)
    {
    case RPLNI_UINT:
        return 1;
    case RPLNI_STR:
        return rplni_scope_add_str(scope, p->value._str);
    case RPLNI_LIST:
        return rplni_scope_add_list(scope, p->value._list);
    case RPLNI_FUNC:
        return rplni_scope_add_func(scope, p->value._func);
    case RPLNI_CLOSURE:
    case RPLNI_BUILTIN:
    default:
        return 0;
    }
}
int rplni_scope_remove_value(struct rplni_scope* scope, struct rplni_value* p)
{
    if (scope == NULL || p == NULL) return 0;

    switch (p->type)
    {
    case RPLNI_UINT:
        return 1;
    case RPLNI_STR:
        return rplni_scope_remove_str(scope, p->value._str);
    case RPLNI_LIST:
        return rplni_scope_remove_list(scope, p->value._list);
    case RPLNI_FUNC:
        return rplni_scope_remove_func(scope, p->value._func);
    case RPLNI_CLOSURE:
    case RPLNI_BUILTIN:
    default:
        return 0;
    }
}
int rplni_scope_add_str(struct rplni_scope* scope, struct rplni_str* p)
{
    return scope != NULL && rplni_ptrlist_add(scope->strs, p);
}
int rplni_scope_has_str(struct rplni_scope* scope, struct rplni_str* p)
{
    return scope != NULL && rplni_ptrlist_has(scope->strs, p);
}
int rplni_scope_remove_str(struct rplni_scope* scope, struct rplni_str* p)
{
    return scope != NULL && rplni_ptrlist_remove(scope->strs, p);
}
int rplni_scope_add_list(struct rplni_scope* scope, struct rplni_list* p)
{
    return scope != NULL && rplni_ptrlist_add(scope->lists, p);
}
int rplni_scope_has_list(struct rplni_scope* scope, struct rplni_list* p)
{
    return scope != NULL && rplni_ptrlist_has(scope->lists, p);
}
int rplni_scope_remove_list(struct rplni_scope* scope, struct rplni_list* p)
{
    return scope != NULL && rplni_ptrlist_remove(scope->lists, p);
}
int rplni_scope_add_func(struct rplni_scope* scope, struct rplni_func* p)
{
    return scope != NULL && rplni_ptrlist_add(scope->funcs, p);
}
int rplni_scope_has_func(struct rplni_scope* scope, struct rplni_func* p)
{
    return scope != NULL && rplni_ptrlist_has(scope->funcs, p);
}
int rplni_scope_remove_func(struct rplni_scope* scope, struct rplni_func* p)
{
    return scope != NULL && rplni_ptrlist_remove(scope->funcs, p);
}
char* rplni_scope_strdup(struct rplni_scope* scope, char* s)
{
    if (s == NULL) return NULL;

    char *s2 = rplni_scope_malloc(scope, strlen(s) + 1);

    if (s2 != NULL)
    {
        strcpy(s2, s);

        return s2;
    }

    rplni_scope_free(scope, s2);

    return NULL;
}

int rplni_state_init(struct rplni_state *state)
{
    if (state == NULL) return 0;

    state->data_stack = NULL;
    state->arg0_stack = NULL;
    state->callee_stack = NULL;
    state->list_head_stack = NULL;
    state->temp_stack = NULL;
    state->scope_stack = NULL;

    if (!rplni_scope_init(&(state->scope))) return 0;

    state->data_stack = rplni_list_new(RPLNI_DATA_STACK_DEFAULT_CAP, &state->scope);
    state->arg0_stack = rplni_list_new(RPLNI_ARG0_STACK_DEFAULT_CAP, &state->scope);
    state->callee_stack = rplni_list_new(RPLNI_CALLEE_STACK_DEFAULT_CAP, &state->scope);
    state->list_head_stack = rplni_list_new(RPLNI_ARG0_STACK_DEFAULT_CAP, &state->scope);
    state->temp_stack = rplni_list_new(RPLNI_TEMP_STACK_DEFAULT_CAP, &state->scope);
    state->scope_stack = rplni_ptrlist_new();

    if (state->data_stack
        && state->arg0_stack && state->callee_stack
        && state->list_head_stack
        && state->temp_stack
        && state->scope_stack) return 1;

    rplni_state_clean(state);
    return 0;
}
int rplni_state_clean(struct rplni_state* state)
{
    if (state == NULL) return 0;

    rplni_scope_clean(&state->scope);

    state->data_stack = NULL;
    state->arg0_stack = NULL;
    state->callee_stack = NULL;
    state->list_head_stack = NULL;
    state->temp_stack = NULL;

    if (state->scope_stack != NULL)
    {
        rplni_ptrlist_del(state->scope_stack);
        state->scope_stack = NULL;
    }

    return 1;
}
int rplni_state_current_scope(struct rplni_state* state, struct rplni_scope** out_scope)
{
    if (state == NULL || out_scope == NULL) return 0;

    size_t len = rplni_ptrlist_len(state->scope_stack);

    if (len > 0)
    {
        *out_scope = state->scope_stack->values.any[len - 1];
    }
    else
    {
        *out_scope = &(state->scope);
    }

    return 1;
}
int rplni_state_outer_scope(struct rplni_state* state, struct rplni_scope** out_scope)
{
    if (state == NULL || out_scope == NULL) return 0;

    size_t len = rplni_ptrlist_len(state->scope_stack);

    if (len == 0) return 0;
    if (len > 1)
    {
        *out_scope = state->scope_stack->values.any[len - 2];
    }
    else
    {
        *out_scope = &(state->scope);
    }

    return 1;
}
int rplni_state_push_scope(struct rplni_state* state, struct rplni_scope* scope)
{
    if (state == NULL || scope == NULL) return 0;

    return rplni_ptrlist_push(state->scope_stack, scope);
}
int rplni_state_pop_scope(struct rplni_state* state, struct rplni_scope** out_scope)
{
    if (state == NULL) return 0;
    if (rplni_ptrlist_len(state->scope_stack) == 0) return 0;

    struct rplni_scope* scope;
    rplni_state_current_scope(state, &scope);
    struct rplni_scope* outer_scope;
    rplni_state_outer_scope(state, &outer_scope);

    for (size_t i = 0; i < state->data_stack->size; ++i)
    {
        struct rplni_value* value = state->data_stack->values + i;

        if (!rplni_scope_has_value(scope, value)) continue;

        rplni_scope_export_value(scope, value, outer_scope);
    }

    return rplni_ptrlist_pop(state->scope_stack, out_scope);
}
int rplni_state_find_var(struct rplni_state* state, char* name, struct rplni_scope** out_scope, size_t* out_index)
{
    if (state == NULL || name == NULL) return 0;

    for (size_t i = rplni_ptrlist_len(state->scope_stack); i-- > 0;)
    {
        struct rplni_scope* scope = state->scope_stack->values.any[i];
        size_t index = rplni_scope_var_index(scope, name);

        if (index >= scope->size) continue;

        if (out_scope != NULL) *out_scope = scope;
        if (out_index != NULL) *out_index = index;
        return 1;
    }

    size_t index = rplni_scope_var_index(&(state->scope), name);

    if (index >= state->scope.size) return 0;

    if (out_scope != NULL) *out_scope = &(state->scope);
    if (out_index != NULL) *out_index = index;
    return 1;
}
int rplni_state_load_var(struct rplni_state* state, char* name, struct rplni_value* out_value)
{
    if (state == NULL || name == NULL || out_value == NULL) return 0;

    struct rplni_scope* scope;
    size_t idx;
    if (!rplni_state_find_var(state, name, &scope, &idx)) return 0;

    return rplni_scope_load_var_by_index(scope, idx, out_value);
}
int rplni_state_store_var(struct rplni_state* state, char* name, struct rplni_value* value)
{
    if (state == NULL || name == NULL || value == NULL) return 0;

    struct rplni_scope* scope;
    size_t idx;
    if (!rplni_state_find_var(state, name, &scope, &idx)) return 0;

    return rplni_scope_store_var_by_index(scope, idx, value);
}
int rplni_state_has_scope(struct rplni_state* state, struct rplni_scope* scope)
{
    if (state == NULL || scope == NULL) return 0;

    return scope == &(state->scope) || rplni_ptrlist_has(state->scope_stack, scope);
}
int rplni_state_compare_scopes(struct rplni_state* state, struct rplni_scope* scope, struct rplni_scope* scope2, int *out_result)
{
    if (state == NULL || scope == NULL || scope2 == NULL) return 0;
    if (!rplni_state_has_scope(state, scope) || !rplni_state_has_scope(state, scope2)) return 0;

    if (scope == scope2)
    {
        *out_result = 0;
        return 1;
    }

    if (scope == &(state->scope)) *out_result = -1;
    else if (scope2 == &(state->scope)) *out_result = 1;
    else
    {
        *out_result = rplni_ptrlist_index(state->scope_stack, scope)
                < rplni_ptrlist_index(state->scope_stack, scope)
            ? -1
            : 1;
    }

    return 1;
}
int rplni_state_push_value(struct rplni_state* state, struct rplni_value* value)
{
    if (state == NULL || value == NULL) return 0;

    return rplni_list_push(state->data_stack, value);
}
int rplni_state_pop_value(struct rplni_state* state, struct rplni_value* out_value)
{
    if (state == NULL || out_value == NULL) return 0;

    return rplni_list_pop(state->data_stack, out_value);
}
int rplni_state_gather_values(struct rplni_state* state, struct rplni_value* value, struct rplni_value* value2)
{
    if (state == NULL || value == NULL || value2 == NULL) return 0;

    struct rplni_scope* owner, *owner2;
    if (!rplni_value_owner(value, &owner) || !rplni_value_owner(value2, &owner2)) return 1;

    if (owner == owner2) return 1;

    int result;
    if (!rplni_state_compare_scopes(state, owner, owner2, &result)) return 0;
    if (result > 0)
    {
        struct rplni_scope* owner3 = owner;
        owner = owner2;
        owner2 = owner3;
        struct rplni_value* value3 = value;
        value = value2;
        value2 = value3;
    }

    return rplni_scope_export_value(owner, value, owner2);
}


void rplni_dump_leaked()
{
    fputs("leaked:\n", stderr);

    for (size_t i = 0; i < rplni_ptrlist_len(all_objs); ++i)
    {
        fprintf(stderr, "  %p\n", all_objs->values.any[i]);
    }
}
