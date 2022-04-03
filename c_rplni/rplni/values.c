#include "rplni.h"
#include "rplni_loader.h"
#include "rplni_macro.h"


#ifndef NDEBUG
struct rplni_ptrlist* all_objs;
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(...)
#endif


int rplni_type_is_number(enum rplni_value_type type)
{
    return type == RPLNI_TYPE_UINT || type == RPLNI_TYPE_SYM;
}
int rplni_type_is_arraylike(enum rplni_value_type type)
{
    return type == RPLNI_TYPE_STR || type == RPLNI_TYPE_LIST || type == RPLNI_TYPE_SLICE;
}
int rplni_type_is_callable(enum rplni_value_type type)
{
    return type == RPLNI_TYPE_UINT || type == RPLNI_TYPE_STR || type == RPLNI_TYPE_LIST || type == RPLNI_TYPE_FUNC || type == RPLNI_TYPE_CLOSURE;
}

int rplni_value_init(struct rplni_value* value)
{
    return rplni_value_init_with_uint(value, 0);
}
int rplni_value_init_with_uint(struct rplni_value* value, uintptr_t uint_value)
{
    if (value == NULL) return 0;

    value->type = RPLNI_TYPE_UINT;
    value->value._uint = uint_value;

    return 1;
}
int rplni_value_init_with_sym(struct rplni_value* value, rplni_id_t id)
{
    if (value == NULL) return 0;

    value->type = RPLNI_TYPE_SYM;
    value->value._sym = id;

    return 1;
}
int rplni_value_init_with_cstr(struct rplni_value* value, size_t size, const char* str_value, struct rplni_state* state)
{
    if (value == NULL || str_value == NULL) return 0;

    struct rplni_str* str = rplni_str_new(size, str_value, state);
    if (str == NULL) return 0;

    value->type = RPLNI_TYPE_STR;
    value->value._str = str;

    return 1;
}
int rplni_value_init_with_captured_list(struct rplni_value* value, size_t size, struct rplni_list *src_stack, struct rplni_state *state)
{
    if (value == NULL) return 0;

    struct rplni_list *list = rplni_list_new_with_captured(size, src_stack, state);
    if (list == NULL) return 0;

    value->type = RPLNI_TYPE_LIST;
    value->value._list = list;

    return 1;
}
int rplni_value_init_with_empty_func(struct rplni_value* value, enum rplni_func_type type, struct rplni_state* state)
{
    if (value == NULL) return 0;

    value->value._func = rplni_func_new(type, state);
    if (value->value._func == NULL)
    {
        value->type = RPLNI_TYPE_UINT;
        return 0;
    }

    value->type = RPLNI_TYPE_FUNC;
    return 1;
}

int rplni_value_clean(struct rplni_value *value)
{
    if (value == NULL) return 0;

    value->type = RPLNI_TYPE_UINT;
    value->value._uint = 0;
    return 1;
}

int rplni_value_eq(const struct rplni_value* value, const struct rplni_value* value2, int unique)
{
    if (value == NULL || value2 == NULL || value->type != value2->type) return 0;

    return value->type == RPLNI_TYPE_STR ?
                value->value._str == value2->value._str
            ||  !unique && !strcmp(value->value._str->value, value2->value._str->value)
        : value->type == RPLNI_TYPE_LIST ? value->value._list == value2->value._list
        : value->type == RPLNI_TYPE_FUNC ? value->value._func == value2->value._func
        : value->type == RPLNI_TYPE_CLOSURE ? value->value._closure == value2->value._closure
        : value->type == RPLNI_TYPE_SYM ? value->value._sym == value2->value._sym
        : value->value._uint == value2->value._uint;
}

int rplni_value_find_living_objs(struct rplni_value* value, struct rplni_ptrlist* known_nodes)
{
    if (value == NULL || known_nodes == NULL) return 0;

    switch (value->type)
    {
    case RPLNI_TYPE_UINT:
    case RPLNI_TYPE_SYM:
        return 0;
    case RPLNI_TYPE_STR:
        return rplni_str_find_living_objs(value->value._str, known_nodes);
    case RPLNI_TYPE_LIST:
        return rplni_list_find_living_objs(value->value._list, known_nodes);
    case RPLNI_TYPE_FUNC:
        return rplni_func_find_living_objs(value->value._func, known_nodes);
    case RPLNI_TYPE_CLOSURE:
        return rplni_closure_find_living_objs(value->value._closure, known_nodes);
    case RPLNI_TYPE_CFUNC:
    default:
        return 0;
    }
}
int rplni_value_owner(struct rplni_value* value, struct rplni_state** out_owner)
{
    if (value == NULL || out_owner == NULL) return 0;
    if (value->type == RPLNI_TYPE_UINT) return 0;

    *out_owner = value->type == RPLNI_TYPE_STR ? value->value._str->owner
        : value->type == RPLNI_TYPE_LIST ? value->value._list->owner
        : value->type == RPLNI_TYPE_FUNC ? value->value._func->owner
        : value->type == RPLNI_TYPE_CLOSURE ? value->value._closure->owner
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
size_t rplni_values_object_index(size_t size, const struct rplni_value* values, const struct rplni_value* value)
{
    if (values == NULL) return 0;
    if (value == NULL) return size;

    size_t idx = 0;
    for (; idx < size && values[idx].type != RPLNI_TYPE_UINT && !rplni_value_eq(values + idx, value, 1); ++idx);

    return idx;
}
int rplni_values_has_object(size_t size, const struct rplni_value* values, const struct rplni_value* value)
{
    if (values == NULL || value == NULL || value->type == RPLNI_TYPE_UINT) return 0;

    size_t idx = rplni_values_object_index(size, values, value);
    return idx < size;
}
int rplni_values_add_object(
        size_t size, struct rplni_value* values,
        struct rplni_value* value,
        struct rplni_state* state,
        size_t *out_size, struct rplni_value* out_values)
{
    if (values == NULL) return 0;

    size_t idx = rplni_values_object_index(size, values, value);
    if (idx >= size)
    {
        size_t size2 = size + RPLNI_LIST_BLOCK_SIZE;
        struct rplni_value* values2 = rplni_state_realloc(state, values, size2 * sizeof(struct rplni_value));

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

size_t rplni_arraylike_size(struct rplni_value* arraylike)
{
    if (arraylike == NULL) return 0;

    switch (arraylike->type)
    {
    case RPLNI_TYPE_UINT:
    case RPLNI_TYPE_SYM:
        return 0;
    case RPLNI_TYPE_STR:
        return arraylike->value._str->size;
    case RPLNI_TYPE_LIST:
        return arraylike->value._list->size;
    case RPLNI_TYPE_SLICE:
        return arraylike->value._iter->end - arraylike->value._iter->start;
    case RPLNI_TYPE_FUNC:
        return 0;
    case RPLNI_TYPE_CLOSURE:
        return 0;
    case RPLNI_TYPE_CFUNC:
    default:
        return 0;
    }
}
size_t rplni_arraylike_start(struct rplni_value* arraylike)
{
    if (arraylike == NULL) return 0;

    switch (arraylike->type)
    {
    case RPLNI_TYPE_UINT:
    case RPLNI_TYPE_SYM:
        return 0;
    case RPLNI_TYPE_STR:
        return 0;
    case RPLNI_TYPE_LIST:
        return 0;
    case RPLNI_TYPE_SLICE:
        return arraylike->value._iter->start;
    case RPLNI_TYPE_FUNC:
        return 0;
    case RPLNI_TYPE_CLOSURE:
        return 0;
    case RPLNI_TYPE_CFUNC:
    default:
        return 0;
    }
}
size_t rplni_arraylike_end(struct rplni_value* arraylike)
{
    if (arraylike == NULL) return 0;

    switch (arraylike->type)
    {
    case RPLNI_TYPE_UINT:
    case RPLNI_TYPE_SYM:
        return 0;
    case RPLNI_TYPE_STR:
        return arraylike->value._str->size;
    case RPLNI_TYPE_LIST:
        return arraylike->value._list->size;
    case RPLNI_TYPE_SLICE:
        return arraylike->value._iter->end;
    case RPLNI_TYPE_FUNC:
        return 0;
    case RPLNI_TYPE_CLOSURE:
        return 0;
    case RPLNI_TYPE_CFUNC:
    default:
        return 0;
    }
}

size_t rplni_callable_argc(struct rplni_value* callable)
{
    if (callable == NULL) return 0;

    switch (callable->type)
    {
    case RPLNI_TYPE_UINT:
        return 1;
    case RPLNI_TYPE_SYM:
        return 0;
    case RPLNI_TYPE_STR:
        return 1;
    case RPLNI_TYPE_LIST:
        return 1;
    case RPLNI_TYPE_FUNC:
        return callable->value._func->params->size;
    case RPLNI_TYPE_CLOSURE:
        return callable->value._closure->funcdef->params->size;
    case RPLNI_TYPE_CFUNC:
    default:
        return 0;
    }
}
int rplni_callable_run(struct rplni_value* callable, struct rplni_state* state)
{
    if (callable == NULL || state == NULL) return 0;

    struct rplni_list* data_stack = state->data_stack;

    switch (callable->type)
    {
        case RPLNI_TYPE_UINT:
            {
                struct rplni_value tmp;
                if (!rplni_list_pop(data_stack, &tmp)) return 0;

                size_t idx = tmp.type == RPLNI_TYPE_UINT ? tmp.value._uint : SIZE_MAX;
                if (idx == SIZE_MAX)
                {
                    rplni_value_clean(&tmp);
                    return 0;
                }

                tmp.value._uint *= callable->value._uint;
                rplni_list_push(data_stack, &tmp);
            }
            return 1;
        case RPLNI_TYPE_SYM:
            {
                struct rplni_value tmp;
                if (!rplni_state_load_var(state, callable->value._sym, &tmp)) return 0;

                rplni_list_push(data_stack, &tmp);
                rplni_value_clean(&tmp);
            }
            return 1;
        case RPLNI_TYPE_STR:
            {
                struct rplni_value tmp;
                if (!rplni_list_pop(data_stack, &tmp)) return 0;

                size_t idx = tmp.type == RPLNI_TYPE_UINT ? tmp.value._uint : SIZE_MAX;
                if (idx == SIZE_MAX)
                {
                    rplni_value_clean(&tmp);
                    return 0;
                }

                if (callable->value._str->size == 0
                    || callable->value._str->size == 1 && idx % 2 == 0)
                {
                    rplni_list_push(data_stack, callable);
                }
                else
                {
                    struct rplni_scope* scope;
                    if (!rplni_state_current_scope(state, &scope)) return 0;

                    rplni_value_init_with_cstr(&tmp, 1, callable->value._str->value + (idx % callable->value._str->size), state);
                    rplni_list_push(data_stack, &tmp);
                    rplni_value_clean(&tmp);
                }
            }
            return 1;
        case RPLNI_TYPE_LIST:
            {
                struct rplni_value tmp;
                if (!rplni_list_pop(data_stack, &tmp)) return 0;

                size_t idx = tmp.type == RPLNI_TYPE_UINT ? tmp.value._uint : SIZE_MAX;
                if (idx == SIZE_MAX)
                {
                    rplni_value_clean(&tmp);
                    return 0;
                }

                struct rplni_value* value;
                if (callable->value._list->size == 0)
                {
                    rplni_value_init(&tmp);
                    value = &tmp;
                }
                else
                {
                    value = callable->value._list->values + (idx % callable->value._list->size);
                }
                rplni_list_push(data_stack, value);
            }
            return 1;
        case RPLNI_TYPE_FUNC:
            return rplni_func_run(callable->value._func, state);
        case RPLNI_TYPE_CLOSURE:
            return rplni_closure_run(callable->value._closure, state);
        case RPLNI_TYPE_CFUNC:
        default:
            return 0;
    }
}


int rplni_named_init(struct rplni_named *named, rplni_id_t id, struct rplni_state* state)
{
    if (named == NULL) return 0;

    named->id = id;

    return rplni_value_init(&(named->value));
}

int rplni_cmd_init(struct rplni_cmd* cmd, enum rplni_op_type op, struct rplni_value *arg, struct rplni_state* state)
{
    if (cmd == NULL) return 0;

    cmd->op = op;
    if (arg != NULL)
    {
        cmd->arg = *arg;
    }
    else
    {
        rplni_value_init(&cmd->arg);
    }

    return 1;
}

int rplni_prog_init(struct rplni_prog* prog, struct rplni_state* state)
{
    if (prog == NULL) return 0;

    prog->cap = RPLNI_LIST_CEILED_SIZE(0);
    prog->size = 0;
    prog->code = rplni_state_malloc(state, prog->cap * sizeof(struct rplni_cmd));

    if (prog->code == NULL)
    {
        prog->cap = 0;
        return 0;
    }

    return 1;
}
int rplni_prog_add(struct rplni_prog* prog, struct rplni_cmd* cmd, struct rplni_state* state)
{
    if (prog->size >= prog->cap)
    {
        size_t cap = prog->cap + RPLNI_LIST_BLOCK_SIZE;
        struct rplni_cmd *code = rplni_state_realloc(state, prog->code, cap * sizeof(struct rplni_cmd));

        if (code == NULL) return 0;

        prog->cap = cap;
        prog->code = code;
    }

    prog->code[prog->size++] = *cmd;

    return 1;
}
int rplni_prog_find_endif(struct rplni_prog* prog, size_t idx, int ignore_els, size_t *out_index)
{
    if (prog == NULL) return 0;

    if (idx >= prog->size)
    {
        idx = prog->size;
    }

    int dpt = 0;
    size_t i = idx;
    for (; i < prog->size; ++i)
    {
        struct rplni_cmd* cmd = prog->code + i;
        if (cmd->op == RPLNI_OP_IF) ++dpt;
        else if (dpt > 0 && cmd->op == RPLNI_OP_ENDIF) --dpt;
        else if (cmd->op == RPLNI_OP_ENDIF || !ignore_els && cmd->op == RPLNI_OP_ELSE)
        {
            break;
        }
    }

    if (out_index != NULL)
    {
        *out_index = i;
    }

    return 1;
}
int rplni_prog_run(struct rplni_prog* prog, struct rplni_state* state, size_t n_params)
{
    if (prog == NULL || state == NULL) return 0;

    struct rplni_scope* scope;
    if (!rplni_state_current_scope(state, &scope)) return 0;

    struct rplni_scope* outer_scope;
    if (!rplni_state_outer_scope(state, &outer_scope)) return 0;

    rplni_state_add_name(state, " args queue");
    rplni_id_t queue_name = rplni_state_id_by_name(state, " args queue");
    rplni_state_add_var(state, queue_name);

    const int gc_interval = state->gc_interval;

    struct rplni_list* data_stack = state->data_stack;
    struct rplni_list* list_head_stack = state->list_head_stack;

    RPLNI_DEF(x);
    RPLNI_DEF(y);
    RPLNI_DEF(z);

#define halt(...) do { fprintf(stderr, __VA_ARGS__); rplni_value_clean(&x); rplni_value_clean(&y); return 0; } while(0)

    for (size_t i = 0; i < prog->size; ++i)
    {
        struct rplni_cmd* cmd = prog->code + i;
        switch (cmd->op)
        {
        case RPLNI_OP_IF:
            if (data_stack->size < 1)
            {
                halt("instruction [?] failed: not enough args\n");
            }

            rplni_list_pop(data_stack, &x);
            if (x.type != RPLNI_TYPE_UINT)
            {
                halt("instruction [?] failed: argument should be a bool in int\n");
            }

            if (x.value._uint != 0) break;

            /* use memorized if not first use */
            if (cmd->arg.value._uint != 0)
            {
                i = cmd->arg.value._uint;
            }
            else
            {
                size_t j;
                rplni_prog_find_endif(prog, i + 1, 0, &j);
                i = j;
                cmd->arg.value._uint = j;
            }
            continue;
        case RPLNI_OP_ELSE:
            if (cmd->arg.value._uint != 0)
            {
                i = cmd->arg.value._uint;
            }
            else
            {
                size_t j;
                rplni_prog_find_endif(prog, i + 1, 1, &j);
                i = j;
                cmd->arg.value._uint = j;
            }
            continue;
        case RPLNI_OP_ENDIF:
            break;
        case RPLNI_OP_RESTART:
            {
                size_t argc = n_params;
                if (data_stack->size < argc)
                {
                    halt("instruction [restart] failed: not enough args for func/%d\n", (int)argc);
                }

                for (size_t n = argc; n-- > 0;)
                {
                    rplni_list_pop(data_stack, &x);
                    rplni_scope_store_var_by_index(scope, n, &x);
                    rplni_value_clean(&x);
                }
            }

            if (state->calls_without_gc++ == gc_interval)
            {
                rplni_state_gc(state);
                state->calls_without_gc = 0;
            }

            i = 0;
            --i;

            break;
        case RPLNI_OP_BEGIN_ARGS:
            if (data_stack->size < 1)
            {
                halt("instruction [(] failed: not enough args\n");
            }
            rplni_list_pop(data_stack, &x);
            if (!rplni_type_is_callable(x.type))
            {
                halt("instruction [(] failed: argument should be callable\n");
            }
            rplni_list_push(state->arg0_stack, &x);

            {
                struct rplni_value tmp;
                rplni_value_init_with_uint(&tmp, data_stack->size);
                rplni_list_push(state->arg0_stack, &tmp);
                rplni_value_clean(&tmp);
            }

            rplni_value_clean(&x);
            break;
        case RPLNI_OP_END_ARGS:
            rplni_list_pop(state->arg0_stack, &x);
            rplni_list_pop(state->arg0_stack, &y);

            if (rplni_type_is_callable(y.type))
            {
                size_t argc = rplni_callable_argc(&y);
                size_t argss_size = data_stack->size - x.value._uint;

                LOG("argc: %d\n", (int)argc);

                if (argc == 0)
                {
                    if (!rplni_callable_run(&y, state))
                    {
                        halt("operator[)] failed\n");
                    }
                }
                else
                {
                    RPLNI_DEF(argss);
                    rplni_value_init_with_captured_list(&argss, argss_size, data_stack, state);
                    rplni_state_store_var(state, queue_name, &argss);

                    if (argss_size % argc != 0)
                    {
                        halt("operator[)] failed: incorrect number of args %d for f/%d\n", (int)argss_size, (int)argc);
                    }

                    for (size_t i = 0; i < argss_size; i += argc)
                    {
                        for (size_t j = 0; j < argc; ++j)
                        {
                            LOG("idx:%d\n", (int)(i + j));
                            struct rplni_value *val = argss.value._list->values + i + j;
                            rplni_list_push(data_stack, val);
                        }

                        if (!rplni_callable_run(&y, state))
                        {
                            halt("operator[)] failed\n");
                        }
                    }

                    rplni_value_clean(&argss);
                }
            }

            rplni_value_clean(&x);
            rplni_value_clean(&y);
            break;
        case RPLNI_OP_PUSH:
            rplni_list_push(data_stack, &(cmd->arg));
            break;
        case RPLNI_OP_PUSH_CLOSURE:
            {
                struct rplni_func* func = cmd->arg.value._func;
                struct rplni_value tmp;
                tmp.type = RPLNI_TYPE_CLOSURE;
                tmp.value._closure = rplni_closure_new(func, state);

                LOG("new closure %p @ %p (%p @ %p)\n",
                    tmp.value._closure, tmp.value._closure->owner,
                    func, func->owner);

                rplni_list_push(data_stack, &tmp);
                rplni_value_clean(&tmp);
            }
            break;
        case RPLNI_OP_DROP:
            if (data_stack->size < 1)
            {
                halt("operator(drop) failed: not enough args\n");
            }

            rplni_list_pop(data_stack, &x);
            rplni_value_clean(&x);
            break;
        case RPLNI_OP_DUP:
            // dup (x -- x x)
            if (data_stack->size < 1)
            {
                halt("operator(dup) failed: not enough args\n");
            }

            rplni_list_pop(data_stack, &x);

            rplni_list_push(data_stack, &x);
            rplni_list_push(data_stack, &x);

            rplni_value_clean(&x);
            break;
        case RPLNI_OP_SWAP:
            if (data_stack->size < 2)
            {
                halt("operator(swap) failed: not enough args\n");
            }

            rplni_list_pop(data_stack, &y);
            rplni_list_pop(data_stack, &x);
            rplni_list_push(data_stack, &y);
            rplni_list_push(data_stack, &x);
            rplni_value_clean(&x);
            rplni_value_clean(&y);
            break;
        case RPLNI_OP_LOAD:
            if (cmd->arg.type == RPLNI_TYPE_UINT)
            {
                size_t idx = cmd->arg.value._uint;
                if (idx < n_params
                        ? !rplni_scope_load_var_by_index(scope, idx, &x)
                        : !rplni_scope_load_var_by_index(outer_scope, idx - n_params, &x))
                {
                    halt("operator(load) failed: failed to load argument(%d)\n", (int)idx);
                }
            }
            else if (cmd->arg.type == RPLNI_TYPE_SYM)
            {
                rplni_id_t id = cmd->arg.value._sym;
                if (!rplni_state_find_var(state, id, NULL, NULL)
                    && !rplni_scope_add_var(scope, id))
                {
                    const char* name;
                    rplni_state_name_by_id(state, id, &name);
                    halt("operator(load) failed: failed to add variable(%s)\n", name);
                }

                if (!rplni_state_load_var(state, id, &x))
                {
                    const char* name;
                    rplni_state_name_by_id(state, id, &name);
                    halt("operator(load) failed: failed to load variable(%s)\n", name);
                }
            }
            else
            {
                halt("operator(load) failed: argument should be any of int and str\n");
            }

            rplni_list_push(data_stack, &x);
            rplni_value_clean(&x);
            break;
        case RPLNI_OP_STORE:
            rplni_list_pop(data_stack, &x);
            if (cmd->arg.type == RPLNI_TYPE_UINT)
            {
                size_t idx = cmd->arg.value._uint;
                if (idx < n_params
                    ? !rplni_scope_store_var_by_index(scope, idx, &x)
                    : !rplni_scope_store_var_by_index(outer_scope, idx - n_params, &x))
                {
                    halt("operator(store) failed: failed to store in parameter(%d)\n", (int)idx);
                }
            }
            else if (cmd->arg.type == RPLNI_TYPE_SYM)
            {
                rplni_id_t id = cmd->arg.value._sym;
                if (!rplni_state_find_var(state, id, NULL, NULL)
                    && !rplni_scope_add_var(scope, id))
                {
                    const char* name;
                    rplni_state_name_by_id(state, id, &name);
                    halt("operator(store) failed: failed to add variable(%s)\n", name);
                }

                if (!rplni_state_store_var(state, id, &x))
                {
                    const char* name;
                    rplni_state_name_by_id(state, id, &name);
                    halt("operator(store) failed: failed to store in variable(%s)\n", name);
                }
            }
            else
            {
                halt("operator(store) failed: argument should be any of int and str\n");
            }
            rplni_value_clean(&x);
            break;
        case RPLNI_OP_BEGIN_LIST:
            // [ (--)
            rplni_value_init_with_uint(&x, data_stack->size);
            rplni_list_push(list_head_stack, &x);
            rplni_value_clean(&x);
            break;
        case RPLNI_OP_END_LIST:
            // ] (-- list)
            if (list_head_stack->size == 0)
            {
                halt("operator(]) failed: ([) was not used\n");
            }
            rplni_list_pop(list_head_stack, &x);
            {
                size_t size = data_stack->size - x.value._uint;
                rplni_value_init_with_captured_list(&y, size, data_stack, state);

                rplni_list_push(data_stack, &y);
                rplni_value_clean(&y);
            }
            rplni_value_clean(&x);
            break;
        case RPLNI_OP_ADD:
            // + (x y -- x+y)
            if (data_stack->size < 2)
            {
                halt("operator(+) failed: not enough args\n");
            }

            rplni_list_pop(data_stack, &y);
            rplni_list_pop(data_stack, &x);

            if (x.type != y.type)
            {
                halt("error. + for different types is not implemented\n");
            }
            else if (x.type == RPLNI_TYPE_UINT)
            {
                x.value._uint += y.value._uint;
                rplni_list_push(data_stack, &x);
            }
            else if (x.type == RPLNI_TYPE_STR)
            {
                RPLNI_DEF_STR(tmp, x.value._str->value, state);

                rplni_str_add(tmp.value._str, y.value._str);
                rplni_list_push(data_stack, &tmp);

                LOG("new str %p @ %p (%p @ %p  +  %p @ %p)\n",
                    tmp.value._str->value, scope,
                    x.value._str->value, x.value._str->owner,
                    y.value._str->value, y.value._str->owner);

                rplni_value_clean(&tmp);
            }
            rplni_value_clean(&x);
            rplni_value_clean(&y);
            break;
        case RPLNI_OP_SUB:
            // - (x y -- x-y)
            if (data_stack->size < 2)
            {
                halt("operator(-) failed: not enough args\n");
            }

            rplni_list_pop(data_stack, &y);
            rplni_list_pop(data_stack, &x);

            if (x.type != y.type || x.type != RPLNI_TYPE_UINT)
            {
                halt("error. - for non-integer values is not implemented\n");
            }
            else
            {
                x.value._uint -= y.value._uint;
                rplni_list_push(data_stack, &x);
            }
            rplni_value_clean(&x);
            rplni_value_clean(&y);
            break;
        case RPLNI_OP_MUL:
            // * (x y -- x*y)
            if (data_stack->size < 2)
            {
                halt("operator(*) failed: not enough args\n");
            }

            rplni_list_pop(data_stack, &y);
            rplni_list_pop(data_stack, &x);

            if (x.type == RPLNI_TYPE_UINT && y.type == RPLNI_TYPE_UINT)
            {
                x.value._uint *= y.value._uint;
                rplni_list_push(data_stack, &x);
            }
            else if (rplni_type_is_callable(x.type) && rplni_type_is_callable(y.type))
            {
                /* this implementation will be replaced with specified structure */

                size_t argc = rplni_callable_argc(&y);
                size_t argc2 = rplni_callable_argc(&x);

                if (argc != argc2 || argc > 1)
                {
                    halt("not implemented composite function type (f/%d g/%d -- fg/%d)?\n", (int)argc, (int)argc2, (int)argc2);
                }

                struct rplni_value func;
                rplni_value_init_with_empty_func(&func, RPLNI_FUNC_FUNC, state);
                for (size_t i = 0; i < argc; ++i)
                {
                    struct rplni_tmpstr* tmpstr = rplni_tmpstr_new();
                    rplni_tmpstr_add_cstr(tmpstr, 6, "param_");
                    rplni_tmpstr_add_ptr(tmpstr, func.value._func);
                    rplni_tmpstr_add_cstr(tmpstr, 1, "_");
                    rplni_tmpstr_add_uintptr(tmpstr, i);

                    rplni_state_add_name(state, tmpstr->s);
                    rplni_id_t id = rplni_state_id_by_name(state, tmpstr->s);
                    rplni_tmpstr_del(tmpstr);

                    rplni_func_add_param(func.value._func, id);
                }

                {
#define prog  func.value._func->prog
#define scope  &tmp_scope
                    if (argc == 0)
                    {
                        CODE_V(RPLNI_OP_PUSH, &y);
                        CODE_I(RPLNI_OP_CALL, 0);
                        CODE_V(RPLNI_OP_PUSH, &x);
                        CODE_I(RPLNI_OP_CALL, 0);
                    }
                    else
                    {
                        CODE_V(RPLNI_OP_PUSH, &x);
                        CODE_I(RPLNI_OP_BEGIN_ARGS, 0);
                        CODE_V(RPLNI_OP_PUSH, &y);
                        CODE_I(RPLNI_OP_BEGIN_ARGS, 0);
                        for (size_t i = 0; i < argc; ++i)
                        {
                            CODE_I(RPLNI_OP_LOAD, i);
                        }
                        CODE_I(RPLNI_OP_END_ARGS, 0);
                        CODE_I(RPLNI_OP_END_ARGS, 0);
                    }
#undef scope
#undef prog
                }

                rplni_list_push(data_stack, &func);
                rplni_value_clean(&func);
            }
            else
            {
                halt("error. [* (%d %d -- ?)] is not implemented\n", (int)x.type, (int)y.type);
            }

            rplni_value_clean(&x);
            rplni_value_clean(&y);
            break;
        case RPLNI_OP_DIV:
            // / (x y -- x/y)
            if (data_stack->size < 2)
            {
                halt("operator(/) failed: not enough args\n");
            }

            rplni_list_pop(data_stack, &y);
            rplni_list_pop(data_stack, &x);

            if (x.type == RPLNI_TYPE_UINT && y.type == RPLNI_TYPE_UINT)
            {
                x.value._uint /= y.value._uint;
                rplni_list_push(data_stack, &x);
            }
            else
            {
                halt("error. / for non-integer values is not implemented\n");
            }

            rplni_value_clean(&x);
            rplni_value_clean(&y);
            break;
        case RPLNI_OP_MOD:
            // % (x y -- x%y)
            if (data_stack->size < 2)
            {
                halt("operator(5) failed: not enough args\n");
            }

            rplni_list_pop(data_stack, &y);
            rplni_list_pop(data_stack, &x);

            if (x.type == RPLNI_TYPE_UINT && y.type == RPLNI_TYPE_UINT)
            {
                x.value._uint %= y.value._uint;
                rplni_list_push(data_stack, &x);
            }
            else
            {
                halt("error. %% for non-integer values is not implemented\n");
            }

            rplni_value_clean(&x);
            rplni_value_clean(&y);
            break;
        case RPLNI_OP_EQ:
            // == (x y -- x==y)
            if (data_stack->size < 2)
            {
                halt("operator(==) failed: not enough args\n");
            }

            rplni_list_pop(data_stack, &y);
            rplni_list_pop(data_stack, &x);

            {
                RPLNI_DEF_UINT(tmp, (uintptr_t)rplni_value_eq(&x, &y, 0));
                rplni_list_push(data_stack, &tmp);
            }

            rplni_value_clean(&x);
            rplni_value_clean(&y);
            break;
        case RPLNI_OP_NEQ:
            // != (x y -- x!=y)
            if (data_stack->size < 2)
            {
                halt("operator(!=) failed: not enough args\n");
            }

            rplni_list_pop(data_stack, &y);
            rplni_list_pop(data_stack, &x);

            {
                RPLNI_DEF_UINT(tmp, !(uintptr_t)rplni_value_eq(&x, &y, 0));
                rplni_list_push(data_stack, &tmp);
            }

            rplni_value_clean(&x);
            rplni_value_clean(&y);
            break;
        case RPLNI_OP_LT:
            // < (x y -- x<y)
            if (data_stack->size < 2)
            {
                halt("operator(<) failed: not enough args\n");
            }

            rplni_list_pop(data_stack, &y);
            rplni_list_pop(data_stack, &x);

            if (x.type != RPLNI_TYPE_UINT || y.type != RPLNI_TYPE_UINT)
            {
                halt("operator(<) failed: < for non-integer values is not implemented \n");
            }

            {
                RPLNI_DEF_UINT(tmp, (uintptr_t)(x.value._uint < y.value._uint));
                rplni_list_push(data_stack, &tmp);
            }

            rplni_value_clean(&x);
            rplni_value_clean(&y);
            break;
        case RPLNI_OP_GT:
            // > (x y -- x<y)
            if (data_stack->size < 2)
            {
                halt("operator(>) failed: not enough args\n");
            }

            rplni_list_pop(data_stack, &y);
            rplni_list_pop(data_stack, &x);

            if (x.type != RPLNI_TYPE_UINT || y.type != RPLNI_TYPE_UINT)
            {
                halt("operator(>) failed: < for non-integer values is not implemented \n");
            }

            {
                RPLNI_DEF_UINT(tmp, (uintptr_t)(x.value._uint > y.value._uint));
                rplni_list_push(data_stack, &tmp);
            }

            rplni_value_clean(&x);
            rplni_value_clean(&y);
            break;
        case RPLNI_OP_RANGE:
            // : (start end -- start ... end-1)
            if (data_stack->size < 2)
            {
                halt("range requires 2 args\n");
            }

            rplni_list_pop(data_stack, &y);
            rplni_list_pop(data_stack, &x);

            if (x.type != RPLNI_TYPE_UINT || y.type != RPLNI_TYPE_UINT)
            {
                halt("range for non-integer args is not implemented\n");
            }

            for (size_t i = x.value._uint; i < y.value._uint; ++i)
            {
                RPLNI_DEF_UINT(tmp, i);
                rplni_list_push(data_stack, &tmp);
                rplni_value_clean(&tmp);
            }

            rplni_value_clean(&x);
            rplni_value_clean(&y);
            break;
        case RPLNI_OP_CALL:
            // call (unary_func -- )
            LOG("try call\n");
            if (data_stack->size < 1)
            {
                halt("operator(call) failed: not enough args\n");
            }

            rplni_list_pop(data_stack, &x);

            if (!rplni_callable_run(&x, state))
            {
                halt("operator(call) failed: failed to call\n");
            }

            rplni_value_clean(&x);
            break;
        case RPLNI_OP_SUM:
            // __sum (list --)
            if (data_stack->size < 1)
            {
                halt("operator(__sum) failed: not enough args\n");
            }

            rplni_list_pop(data_stack, &x);
            if (!rplni_type_is_arraylike(x.type))
            {
                halt("operator(__sum) failed: argument should be iterable\n");
            }

            if (x.type != RPLNI_TYPE_LIST)
            {
                halt("iter is not implemented\n");
            }
            else
            {
                struct rplni_list* list = x.value._list;

                if (list->size == 0)
                {
                    RPLNI_DEF_UINT(tmp, 0);
                    rplni_list_push(data_stack, &tmp);
                }
                else if (list->values->type == RPLNI_TYPE_UINT)
                {
                    RPLNI_DEF_UINT(tmp, 0);
                    for (size_t i = 0; i < list->size; ++i)
                    {
                        struct rplni_value* value = list->values + i;
                        if (value->type != RPLNI_TYPE_UINT) continue;

                        tmp.value._uint += value->value._uint;
                    }
                    rplni_list_push(data_stack, &tmp);
                }
                else if (list->values->type == RPLNI_TYPE_STR)
                {
                    struct rplni_tmpstr* tmpstr = rplni_tmpstr_new();
                    for (size_t i = 0; i < list->size; ++i)
                    {
                        struct rplni_value* value = list->values + i;
                        rplni_tmpstr_add_value(tmpstr, value, NULL);
                    }

                    struct rplni_value tmp;
                    rplni_value_init_with_cstr(&tmp, tmpstr->size, tmpstr->s, state);
                    rplni_tmpstr_del(tmpstr);

                    rplni_list_push(data_stack, &tmp);
                    rplni_value_clean(&tmp);
                }
            }

            rplni_value_clean(&x);
            break;
        case RPLNI_OP_STR:
            // __str (any -- str)
            if (data_stack->size < 1)
            {
                halt("operator(__str) failed: not enough args\n");
            }

            rplni_list_pop(data_stack, &x);

            {
                struct rplni_tmpstr* tmpstr;
                if (rplni_value_to_tmpstr(&x, &tmpstr))
                {
                    rplni_value_init_with_cstr(&y, tmpstr->size, tmpstr->s, state);

                    rplni_tmpstr_del(tmpstr);
                }
                else
                {
                    rplni_value_init_with_cstr(&y, 0, "", state);
                }
            }

            rplni_list_push(data_stack, &y);

            rplni_value_clean(&x);
            rplni_value_clean(&y);
            break;
        case RPLNI_OP_INT:
            // __int (str -- int)
            if (data_stack->size < 1)
            {
                halt("operator(__int) failed: not enough args\n");
            }

            rplni_list_pop(data_stack, &x);
            if (x.type == RPLNI_TYPE_STR)
            {
                rplni_value_init_with_uint(&y, (uintptr_t)atol(x.value._str->value));
                rplni_list_push(data_stack, &y);

                rplni_value_clean(&y);
            }
            else if (x.type == RPLNI_TYPE_UINT)
            {
                rplni_list_push(data_stack, &x);
            }
            else
            {
                halt("operator(__int) failed: not implemented\n");
            }

            rplni_value_clean(&x);
            break;
        case RPLNI_OP_PRINT:
            // __print (msg --)
            if (data_stack->size < 1)
            {
                halt("operator(__print) failed: not enough args\n");
            }

            rplni_list_pop(data_stack, &x);

            {
                struct rplni_tmpstr* tmpstr;
                if (rplni_value_to_tmpstr(&x, &tmpstr))
                {
                    puts(tmpstr->s);

                    rplni_tmpstr_del(tmpstr);
                }
            }

            rplni_value_clean(&x);
            break;
        case RPLNI_OP_LEN:
            // __len (list -- length)
            if (data_stack->size < 1)
            {
                halt("operator(__len) failed: not enough args\n");
            }

            rplni_list_pop(data_stack, &x);

            if (!rplni_type_is_arraylike(x.type))
            {
                halt("operator(__len) failed: arg is not a arraylike\n");
            }

            rplni_value_init_with_uint(&y, (uintptr_t)rplni_arraylike_size(&x));
            rplni_list_push(data_stack, &y);

            rplni_value_clean(&x);
            rplni_value_clean(&y);
            break;
        case RPLNI_OP_LIST_POP:
            // pop
            // @ (list -- val list)
            if (data_stack->size < 1)
            {
                halt("operator(@) failed: not enough args\n");
            }

            rplni_list_pop(data_stack, &x);

            if (x.type != RPLNI_TYPE_LIST)
            {
                halt("operator(@) failed: arg is not a list\n");
            }

            rplni_list_pop(x.value._list, &y);

            rplni_list_push(data_stack, &y);
            rplni_list_push(data_stack, &x);

            rplni_value_clean(&x);
            rplni_value_clean(&y);
            break;
        case RPLNI_OP_LIST_PUSH:
            // push
            // ! (val obj -- obj)
            if (data_stack->size < 2)
            {
                halt("instruction(list_push) failed: not enough args\n");
            }

            rplni_list_pop(data_stack, &y);
            rplni_list_pop(data_stack, &x);

            if (x.type != RPLNI_TYPE_LIST)
            {
                halt("instruction(list_push) failed: arg is not a list\n");
            }

            rplni_list_push(x.value._list, &y);
            rplni_list_push(data_stack, &x);

            rplni_value_clean(&x);
            rplni_value_clean(&y);
            break;
        case RPLNI_OP_SPREAD:
            // .. (list -- list(0 len(list):))
            if (data_stack->size < 1)
            {
                halt("operator(..) failed: not enough args\n");
            }

            rplni_list_pop(data_stack, &x);

            if (!rplni_type_is_arraylike(x.type))
            {
                halt("operator(..) failed: arg is not an arraylike\n");
            }

            for (size_t i = rplni_arraylike_start(&x); i < rplni_arraylike_end(&x); ++i)
            {
                struct rplni_value arg;
                rplni_value_init_with_uint(&arg, i);

                rplni_list_push(data_stack, &arg);
                if (!rplni_callable_run(&x, state))
                {
                    halt("operator(..) failed");
                }

                rplni_value_clean(&arg);
            }

            rplni_value_clean(&x);
            break;
        case RPLNI_OP_OPTIMIZE:
            // __optimize (func -- func)
            if (data_stack->size < 1)
            {
                halt("instruction(__optimize) failed: not enough args\n");
            }

            rplni_list_pop(data_stack, &x);

            if (x.type == RPLNI_TYPE_FUNC)
            {
                rplni_prog_optimize(&x.value._func->prog, state);
            }
            else if (x.type == RPLNI_TYPE_CLOSURE)
            {
                rplni_prog_optimize(&x.value._closure->funcdef->prog, state);
            }
            else
            {
                halt("instruction(__optimize) failed: arg is not a func\n");
            }

            rplni_list_push(data_stack, &x);

            rplni_value_clean(&x);
            break;
        case RPLNI_OP_COMPILE:
            // __compile (lang func_name func -- source2)
            if (data_stack->size < 3)
            {
                halt("__compile failed: requires 3 args\n");
            }

            rplni_list_pop(data_stack, &z);
            rplni_list_pop(data_stack, &y);
            rplni_list_pop(data_stack, &x);

            if (x.type != RPLNI_TYPE_STR || y.type != RPLNI_TYPE_STR)
            {
                halt("__compile failed: language name and function name should be a string\n");
            }

            if (z.type == RPLNI_TYPE_FUNC)
            {
                rplni_prog_compile(&z.value._func->prog, x.value._str->value, state);
            }
            else
            {
                halt("__compile failed: function name should be a string\n");
            }

            rplni_value_clean(&x);
            rplni_value_clean(&y);
            rplni_value_clean(&z);
            break;
        case RPLNI_OP_DEBUG:
            puts("---- ---- ---- ---- ---- ---- ---- ----");
            fputs("data: ...", stdout);
            for (size_t i = 0; i < data_stack->size; ++i)
            {
                printf(" %d", (int)data_stack->values[i].type);
            }
            fputs("\nscps: ...", stdout);
            for (size_t i = 0; i < state->scope_stack->size; ++i)
            {
                printf(" %p", state->scope_stack->values.any[i]);
            }
            puts("");
            break;
        case RPLNI_OP_NOP:
            break;
        default:
            halt("unknown op(%d)\n", (int)cmd->op);
        }
    }

#undef halt

    if (state->calls_without_gc++ == gc_interval)
    {
        rplni_state_gc(state);
        state->calls_without_gc = 0;
    }

    return 1;
}
int rplni_prog_optimize(struct rplni_prog* prog, struct rplni_state* state)
{
    if (prog == NULL || state == NULL) return 0;

    struct rplni_value empty;
    rplni_value_init_with_uint(&empty, 0);

    for (size_t i = 0; i < prog->size - 2; ++i)
    {
        struct rplni_cmd* cmd = prog->code + i;
        struct rplni_cmd* cmd2 = prog->code + i + 1;
        struct rplni_cmd* cmd3 = prog->code + i + 2;

        switch (cmd->op)
        {
        case RPLNI_OP_SWAP:
            if (cmd2->op == RPLNI_OP_SWAP)
            {
                cmd->op = RPLNI_OP_NOP;
                rplni_value_clean(&cmd->arg);
                cmd2->op = RPLNI_OP_NOP;
                rplni_value_clean(&cmd2->arg);
                ++i;
            }
            break;
        case RPLNI_OP_DUP:
            if (cmd2->op == RPLNI_OP_DROP)
            {
                cmd->op = RPLNI_OP_NOP;
                rplni_value_clean(&cmd->arg);
                cmd2->op = RPLNI_OP_NOP;
                rplni_value_clean(&cmd2->arg);
                ++i;
            }
            break;
        case RPLNI_OP_PUSH:
            if (cmd->arg.type == RPLNI_TYPE_UINT && cmd->arg.value._uint == 0
                && cmd2->op == RPLNI_OP_NEQ
                && cmd3->op == RPLNI_OP_IF)
            {
                cmd->op = RPLNI_OP_NOP;
                rplni_value_clean(&cmd->arg);
                cmd2->op = RPLNI_OP_NOP;
                rplni_value_clean(&cmd2->arg);
                ++i;
            }
            else if (cmd2->op == RPLNI_OP_DROP)
            {
                cmd->op = RPLNI_OP_NOP;
                rplni_value_clean(&cmd->arg);
                cmd2->op = RPLNI_OP_NOP;
                rplni_value_clean(&cmd2->arg);
                ++i;
            }
            else if (cmd->arg.type == RPLNI_TYPE_UINT
                && cmd2->op == RPLNI_OP_PUSH && cmd2->arg.type == RPLNI_TYPE_UINT
                && (
                    cmd3->op == RPLNI_OP_ADD
                    || cmd3->op == RPLNI_OP_SUB
                    || cmd3->op == RPLNI_OP_MUL
                    || cmd3->op == RPLNI_OP_MOD))
            {
                uintptr_t n =
                    cmd3->op == RPLNI_OP_ADD ? (cmd->arg.value._uint + cmd2->arg.value._uint)
                    : cmd3->op == RPLNI_OP_SUB ? (cmd->arg.value._uint - cmd2->arg.value._uint)
                    : cmd3->op == RPLNI_OP_MUL ? (cmd2->arg.value._uint ? cmd->arg.value._uint * cmd2->arg.value._uint : 0)
                    : cmd3->op == RPLNI_OP_DIV ? (cmd2->arg.value._uint ? cmd->arg.value._uint / cmd2->arg.value._uint : 0)
                    : /* RPLNI_OP_MOD */ (cmd2->arg.value._uint ? cmd->arg.value._uint % cmd2->arg.value._uint : 0);

                cmd->op = RPLNI_OP_NOP;
                rplni_value_clean(&cmd->arg);
                cmd2->op = RPLNI_OP_NOP;
                rplni_value_clean(&cmd2->arg);

                cmd3->op = RPLNI_OP_PUSH;
                cmd3->arg.type = RPLNI_TYPE_UINT;
                cmd3->arg.value._uint = n;

                ++i;
            }
            break;
        case RPLNI_OP_BEGIN_LIST:
            if (cmd2->op == RPLNI_OP_END_LIST)
            {
                cmd->op = RPLNI_OP_NOP;
                cmd->arg = empty;

                cmd2->op = RPLNI_OP_PUSH;
                cmd2->arg.type = RPLNI_TYPE_LIST;
                cmd2->arg.value._list = rplni_list_new(0, state);

                ++i;
            }
        case RPLNI_OP_BEGIN_ARGS:
            if ((cmd2->op == RPLNI_OP_PUSH || cmd2->op == RPLNI_OP_PUSH_CLOSURE)
                && cmd3->op == RPLNI_OP_END_ARGS)
            {
                *cmd = *cmd2;

                cmd2->op = RPLNI_OP_SWAP;
                cmd2->arg = empty;

                cmd3->op = RPLNI_OP_CALL;
                cmd3->arg = empty;

                i += 2;
            }
            else if (cmd2->op == RPLNI_OP_END_ARGS)
            {
                cmd->op = RPLNI_OP_CALL;
                cmd->arg = empty;

                cmd2->op = RPLNI_OP_NOP;
                cmd2->arg = empty;

                ++i;
            }
            break;
        }
    }

    return 1;
}
int rplni_prog_compile(struct rplni_prog* prog,  const char *lang, struct rplni_state* state)
{
    if (prog == NULL || lang == NULL || state == NULL) return 0;
    if (strcmp(lang, "C(rplni)")) return 0;

    struct rplni_tmpstr *tmp = rplni_tmpstr_new();
    if (tmp == NULL) return 0;

    int err = 0;
    for (size_t i = 0; i < prog->size; ++i)
    {
        struct rplni_cmd *cmd = prog->code + i;
        switch (cmd->op)
        {
            case RPLNI_OP_PUSH:
                if (cmd->arg.type == RPLNI_TYPE_UINT)
                {
                    rplni_tmpstr_add_cstr(tmp, 23, "rplni_push_uint(state, ");
                    rplni_tmpstr_add_uintptr(tmp, cmd->arg.value._uint);
                    rplni_tmpstr_add_cstr(tmp, 3, ");\n");

                    break;
                }
                else if (cmd->arg.type == RPLNI_TYPE_STR)
                {
                    rplni_tmpstr_add_cstr(tmp, 23, "rplni_push_cstr(state, ");
                    rplni_tmpstr_add_uintptr(tmp, cmd->arg.value._str->size);
                    rplni_tmpstr_add_cstr(tmp, 2, ", ");
                    rplni_tmpstr_add_str(tmp, cmd->arg.value._str);
                    rplni_tmpstr_add_cstr(tmp, 3, ");\n");

                    break;
                }

                err = 1;
                i = prog->size;
                break;
            case RPLNI_OP_DUP:
                rplni_tmpstr_add_cstr(tmp, 18, "rplni_dup(state);\n");
                break;
            case RPLNI_OP_SWAP:
                rplni_tmpstr_add_cstr(tmp, 19, "rplni_swap(state);\n");
                break;
            case RPLNI_OP_DROP:
                rplni_tmpstr_add_cstr(tmp, 19, "rplni_drop(state);\n");
                break;
            case RPLNI_OP_ADD:
                rplni_tmpstr_add_cstr(tmp, 18, "rplni_add(state);\n");
                break;
            case RPLNI_OP_SUB:
                rplni_tmpstr_add_cstr(tmp, 18, "rplni_sub(state);\n");
                break;
            case RPLNI_OP_MUL:
                rplni_tmpstr_add_cstr(tmp, 18, "rplni_mul(state);\n");
                break;
            case RPLNI_OP_DIV:
                rplni_tmpstr_add_cstr(tmp, 18, "rplni_div(state);\n");
                break;
            case RPLNI_OP_MOD:
                rplni_tmpstr_add_cstr(tmp, 18, "rplni_mod(state);\n");
                break;
            case RPLNI_OP_IF:
                rplni_tmpstr_add_cstr(tmp, 25, "if (rplni_cond(state)) {\n");
                break;
            case RPLNI_OP_ELSE:
                rplni_tmpstr_add_cstr(tmp, 9, "} else {\n");
                break;
            case RPLNI_OP_ENDIF:
                rplni_tmpstr_add_cstr(tmp, 2, "}\n");
                break;
            case RPLNI_OP_NOP:
                break;
            default:
                err = 1;
                i = prog->size;
                break;
        }
    }

    if (!err)
    {
        struct rplni_value value;
        if (rplni_value_init_with_cstr(&value, tmp->size, tmp->s, state))
        {
            struct rplni_list *data_stack = state->data_stack;
            rplni_list_push(data_stack, &value);
            rplni_value_clean(&value);
        }
        else
        {
            err = 1;
        }
    }

    rplni_tmpstr_del(tmp);
    return !err;
}

int rplni_scope_init(struct rplni_scope *scope, struct rplni_state *state)
{
    if (scope == NULL || state == NULL) return 0;

    scope->owner = state;
    scope->cap = 16;
    scope->size = 0;
    scope->vars = rplni_state_malloc(state, scope->cap * sizeof(struct rplni_named));

    return scope->vars != NULL;
}
int rplni_scope_clean(struct rplni_scope* scope)
{
    if (scope == NULL) return 0;

    memset(scope, 0, sizeof(struct rplni_scope));
    return 1;
}
int rplni_scope_find_living_objs(struct rplni_scope* scope, struct rplni_ptrlist* known_nodes)
{
    if (scope == NULL || known_nodes == NULL) return 0;

    rplni_ptrlist_add(known_nodes, scope->vars, NULL);

    for (size_t i = 0; i < scope->size; ++i)
    {
        struct rplni_named* named = scope->vars + i;

        rplni_value_find_living_objs(&named->value, known_nodes);
    }

    return 1;
}

void *rplni_state_malloc(struct rplni_state* state, size_t size)
{
    void* p = malloc(size);

#ifndef NDEBUG
    if (state != NULL) rplni_ptrlist_add(all_objs, p, NULL);
#endif

    LOG("+%p @ %p (size: %d)\n", p, state, (int)size);

    if (state == NULL) return p;
    if (rplni_ptrlist_add(state->objs, p, NULL)) return p;

    free(p);
    return NULL;
}
void* rplni_state_realloc(struct rplni_state* state, void *p, size_t size)
{
    LOG("-%p @ %p (new_size: %d)\n", p, state, (int)size);
    if (p == NULL) return NULL;

#ifndef NDEBUG
    assert(state == NULL || rplni_ptrlist_has(all_objs, p));
    if (state != NULL) rplni_ptrlist_remove(all_objs, p, NULL);
#endif

    if (state != NULL && !rplni_state_has(state, p)) return NULL;

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

    LOG("+%p @ %p (reallocated)\n", q, state);

#ifndef NDEBUG
    if (state != NULL) rplni_ptrlist_add(all_objs, q, NULL);
#endif

    if (state == NULL || p == q) return q;

    if (rplni_ptrlist_remove(state->objs, p, NULL)
        && rplni_ptrlist_add(state->objs, q, NULL))
    {
        return q;
    }

    free(q);
    return NULL;
}
int rplni_state_free(struct rplni_state* state, void* p)
{
    LOG("-%p @ %p\n", p, state);
    if (p == NULL) return 0;

#ifndef NDEBUG
    assert(state == NULL || rplni_ptrlist_has(all_objs, p));
    if (state != NULL) rplni_ptrlist_remove(all_objs, p, NULL);
#endif

    if (state == NULL || rplni_ptrlist_remove(state->objs, p, NULL))
    {
        free(p);

        return 1;
    }

    LOG("failed -%p @ %p\n", p, state);
    return 0;
}
int rplni_scope_add_var(struct rplni_scope* scope, rplni_id_t id)
{
    if (scope == NULL) return 0;
    if (rplni_scope_has_var(scope, id)) return 0;

    if (scope->size >= scope->cap)
    {
        size_t cap = scope->cap + RPLNI_LIST_BLOCK_SIZE;
        struct rplni_named* vars = rplni_state_realloc(scope->owner, scope->vars, cap * sizeof(struct rplni_named));

        if (vars == NULL) return 0;

        scope->cap = cap;
        scope->vars = vars;
    }

    const char* name = scope->owner->loader.names->values.cstr[id];
    LOG("new var[%s] @ %p\n", name, scope);
    if (!rplni_named_init(scope->vars + scope->size, id, scope->owner)) return 0;

    scope->size++;
    return 1;
}
int rplni_scope_add_var_by_name(struct rplni_scope* scope, char* name)
{
    if (scope == NULL || name == NULL) return 0;
    if (!rplni_state_has_name(scope->owner, name) && !rplni_state_add_name(scope->owner, name)) return 0;

    rplni_id_t id = rplni_state_id_by_name(scope->owner, name);
    return rplni_scope_add_var(scope, id);
}
size_t rplni_scope_var_index(const struct rplni_scope* scope, rplni_id_t id)
{
    if (scope == NULL) return 0;

    size_t i = 0;
    for (; i < scope->size; ++i)
    {
        if (scope->vars[i].id == id) return i;
    }

    return i;
}
int rplni_scope_has_var(const struct rplni_scope* scope, rplni_id_t id)
{
    return rplni_scope_var_index(scope, id) < scope->size;
}
int rplni_scope_load_var_by_index(struct rplni_scope* scope, size_t idx, struct rplni_value* out_value)
{
    if (scope == NULL || out_value == NULL) return 0;
    if (idx >= scope->size) return 0;

    struct rplni_named* v = scope->vars + idx;
    *out_value = v->value;
    return 1;
}
int rplni_scope_load_var(struct rplni_scope* scope, rplni_id_t id, struct rplni_value* out_value)
{
    if (scope == NULL || out_value == NULL) return 0;
    size_t idx = rplni_scope_var_index(scope, id);

    return rplni_scope_load_var_by_index(scope, idx, out_value);
}
int rplni_scope_store_var_by_index(struct rplni_scope* scope, size_t idx, struct rplni_value* value)
{
    if (scope == NULL || value == NULL) return 0;
    if (idx >= scope->size) return 0;

    struct rplni_named* v = scope->vars + idx;
    v->value = *value;
    return 1;
}
int rplni_scope_store_var(struct rplni_scope* scope, rplni_id_t id, struct rplni_value* value)
{
    if (scope == NULL || value == NULL) return 0;

    size_t idx = rplni_scope_var_index(scope, id);

    return rplni_scope_store_var_by_index(scope, idx, value);
}
int rplni_scope_store_var_by_name(struct rplni_scope* scope, const char* name, struct rplni_value* value)
{
    if (scope == NULL || name == NULL) return 0;
    if (!rplni_state_has_name(scope->owner, name)) return 0;

    rplni_id_t id = rplni_state_id_by_name(scope->owner, name);
    return rplni_scope_store_var(scope, id, value);
}

static int rplni_state_dealloc_all(struct rplni_state* state, struct rplni_ptrlist* objs);
int rplni_state_init(struct rplni_state *state)
{
    if (state == NULL) return 0;

#ifndef NDEBUG
    all_objs = rplni_ptrlist_new(1, NULL);
#endif

    state->calls_without_gc = 0;
    state->gc_interval = RPLNI_GC_DEFAULT_INTERVAL;

    state->data_stack = NULL;
    state->arg0_stack = NULL;
    state->callee_stack = NULL;
    state->list_head_stack = NULL;
    state->scope_stack = NULL;

    state->objs = rplni_ptrlist_new(1, NULL);
    if (state->objs == NULL) return 0;

    if (!rplni_loader_init(&(state->loader))) return 0;
    if (!rplni_scope_init(&(state->scope), state)) return 0;

    state->data_stack = rplni_list_new(RPLNI_DATA_STACK_DEFAULT_CAP, NULL);
    state->arg0_stack = rplni_list_new(RPLNI_ARG0_STACK_DEFAULT_CAP, NULL);
    state->callee_stack = rplni_list_new(RPLNI_CALLEE_STACK_DEFAULT_CAP, NULL);
    state->list_head_stack = rplni_list_new(RPLNI_LIST_HEAD_STACK_DEFAULT_CAP, NULL);
    state->scope_stack = rplni_ptrlist_new(0, NULL);

    if (state->data_stack
        && state->arg0_stack && state->callee_stack
        && state->list_head_stack
        && state->scope_stack
        && state->objs
        && rplni_state_push_scope(state, &state->scope)
        ) return 1;

    rplni_state_clean(state);

#ifndef NDEBUG
    rplni_dump_leaked();
    rplni_ptrlist_del(all_objs, NULL);
#endif

    return 0;
}
int rplni_state_clean(struct rplni_state* state)
{
    if (state == NULL) return 0;

    rplni_state_dealloc_all(state, state->objs);
    rplni_ptrlist_del(state->objs, NULL);
    rplni_scope_clean(&state->scope);
    rplni_loader_clean(&state->loader);

    memset(state, 0, sizeof(struct rplni_state));

#ifndef NDEBUG
    rplni_dump_leaked();
#endif

    return 1;
}
int rplni_state_gc(struct rplni_state* state)
{
    if (state == NULL) return 0;

    struct rplni_ptrlist* known_nodes = rplni_ptrlist_new(1, NULL);

    rplni_list_find_living_objs(state->data_stack, known_nodes);
    rplni_list_find_living_objs(state->arg0_stack, known_nodes);
    rplni_list_find_living_objs(state->list_head_stack, known_nodes);

    for (size_t i = 0; i < state->scope_stack->size; ++i)
    {
        rplni_scope_find_living_objs(state->scope_stack->values.any[i], known_nodes);
    }

    for (size_t i = 0; i < known_nodes->size; ++i)
    {
        rplni_ptrlist_remove(state->objs, known_nodes->values.any[i], NULL);
    }

    rplni_state_dealloc_all(state, state->objs);
    rplni_state_free(NULL, state->objs);

    state->objs = known_nodes;

    return 1;
}
static int rplni_state_dealloc_all(struct rplni_state* state, struct rplni_ptrlist* objs)
{
    if (state == NULL || objs == NULL) return 0;

    for (size_t i = objs->size; i-- > 0;)
    {
        rplni_state_free(state, objs->values.any[i]);
    }

    return 1;
}
int rplni_state_add(struct rplni_state* state, void* p)
{
    return state != NULL && rplni_ptrlist_add(state->objs, p, state);
}
int rplni_state_has(const struct rplni_state* state, const void* p)
{
    return state != NULL && rplni_ptrlist_has(state->objs, p);
}
int rplni_state_remove(struct rplni_state* state, void* p)
{
    return state != NULL && rplni_ptrlist_remove(state->objs, p, state);
}
char* rplni_state_strdup(struct rplni_state* state, size_t size, const char* s)
{
    if (s == NULL) return NULL;

    char* s2 = rplni_state_malloc(state, size + 1);
    if (s2 == NULL) return NULL;

    strncpy(s2, s, size);
    s2[size] = 0;

    return s2;
}
int rplni_state_current_scope(struct rplni_state* state, struct rplni_scope** out_scope)
{
    if (state == NULL || out_scope == NULL) return 0;

    size_t len = state->scope_stack->size;

    if (len == 0) return 0;

    *out_scope = state->scope_stack->values.any[len - 1];
    return 1;
}
int rplni_state_outer_scope(struct rplni_state* state, struct rplni_scope** out_scope)
{
    if (state == NULL || out_scope == NULL) return 0;

    size_t len = state->scope_stack->size;

    if (len < 2)
    {
        *out_scope = &state->scope;
    }
    else
    {
        *out_scope = state->scope_stack->values.any[len - 2];
    }
    return 1;
}
int rplni_state_push_scope(struct rplni_state* state, struct rplni_scope* scope)
{
    if (state == NULL || scope == NULL) return 0;

    return rplni_ptrlist_push(state->scope_stack, scope, NULL);
}
int rplni_state_pop_scope(struct rplni_state* state, struct rplni_scope** out_scope)
{
    if (state == NULL) return 0;
    if (state->scope_stack->size < 2) return 0;

    if (out_scope != NULL) return rplni_ptrlist_pop(state->scope_stack, out_scope, NULL);

    return rplni_ptrlist_pop(state->scope_stack, NULL, NULL);
}
int rplni_state_find_var(struct rplni_state* state, rplni_id_t id, struct rplni_scope** out_scope, size_t* out_index)
{
    if (state == NULL) return 0;

    for (size_t i = state->scope_stack->size; i-- > 0;)
    {
        struct rplni_scope* scope = state->scope_stack->values.any[i];
        size_t index = rplni_scope_var_index(scope, id);

        if (index >= scope->size) continue;

        if (out_scope != NULL) *out_scope = scope;
        if (out_index != NULL) *out_index = index;
        return 1;
    }

    return 0;
}
int rplni_state_load_var(struct rplni_state* state, rplni_id_t id, struct rplni_value* out_value)
{
    if (state == NULL || out_value == NULL) return 0;

    struct rplni_scope* scope;
    size_t idx;
    if (!rplni_state_find_var(state, id, &scope, &idx)) return 0;

    return rplni_scope_load_var_by_index(scope, idx, out_value);
}
int rplni_state_store_var(struct rplni_state* state, rplni_id_t id, struct rplni_value* value)
{
    if (state == NULL || value == NULL) return 0;

    struct rplni_scope* scope;
    size_t idx;
    if (!rplni_state_find_var(state, id, &scope, &idx)) return 0;

    return rplni_scope_store_var_by_index(scope, idx, value);
}
int rplni_state_add_var(struct rplni_state* state, rplni_id_t id)
{
    if (state == NULL) return 0;

    struct rplni_scope* scope;
    if (!rplni_state_current_scope(state, &scope)) return 0;

    return rplni_scope_add_var(scope, id);
}
int rplni_state_has_scope(const struct rplni_state* state, const struct rplni_scope* scope)
{
    if (state == NULL || scope == NULL) return 0;

    return scope == &(state->scope) || rplni_ptrlist_has(state->scope_stack, scope);
}
int rplni_state_add_name(struct rplni_state* state, char *name)
{
    return rplni_loader_add_name(&state->loader, name, 1);
}
int rplni_state_has_name(const struct rplni_state* state, const char *name)
{
    return rplni_loader_has_name(&state->loader, name);
}
rplni_id_t rplni_state_id_by_name(const struct rplni_state* state, const char* name)
{
    return rplni_loader_name_index(&state->loader, name);
}
int rplni_state_name_by_id(const struct rplni_state* state, rplni_id_t id, const char** out_name)
{
    return rplni_loader_name_by_index(&state->loader, id, out_name);
}

int rplni_state_compare_scopes(
    const struct rplni_state* state,
    const struct rplni_scope* scope,
    const struct rplni_scope* scope2,
    int *out_result)
{
    if (state == NULL || scope == NULL || scope2 == NULL) return 0;

    if (!rplni_state_has_scope(state, scope) && !rplni_state_has_scope(state, scope2))
    {
        *out_result = 0;
        return 1;
    }

    if (!rplni_state_has_scope(state, scope))
    {
        *out_result = 1;
        return 1;
    }
    if (!rplni_state_has_scope(state, scope2))
    {
        *out_result = -1;
        return 1;
    }

    if (scope == scope2)
    {
        *out_result = 0;
        return 1;
    }

#ifndef NDEBUG
    assert(rplni_ptrlist_has(state->scope_stack, scope));
    assert(rplni_ptrlist_has(state->scope_stack, scope2));
#endif
    *out_result = rplni_ptrlist_index(state->scope_stack, scope)
            < rplni_ptrlist_index(state->scope_stack, scope2)
        ? -1
        : 1;

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
int rplni_state_add_argv(struct rplni_state* state, int argc, char *argv[])
{
    if (state == NULL || argv == NULL) return 0;

    struct rplni_scope *scope;
    if (!rplni_state_current_scope(state, &scope)) return 0;

    struct rplni_value v;
    v.type = RPLNI_TYPE_LIST;
    v.value._list = rplni_list_new(argc, state);
    for (int i = 0; i < argc; ++i)
    {
        struct rplni_value arg;
        rplni_value_init_with_cstr(&arg, strlen(argv[1]), argv[i], state);
        rplni_list_push(v.value._list, &arg);
        rplni_value_clean(&arg);
    }
    rplni_scope_add_var_by_name(scope, "argv");
    rplni_scope_store_var_by_name(scope, "argv", &v);
    rplni_value_clean(&v);

    return 1;
}
int rplni_state_add_builtin_func(struct rplni_state* state, char *name, enum rplni_op_type op, size_t argc)
{
    if (state == NULL || name == NULL) return 0;

    if (!rplni_state_add_name(state, name)) return 0;
    rplni_id_t id = rplni_state_id_by_name(state, name);

    struct rplni_scope *scope;
    if (!rplni_state_current_scope(state, &scope)) return 0;

    struct rplni_value v;
    if (!rplni_value_init_with_empty_func(&v, RPLNI_FUNC_FUNC, state)) return 0;

    char *params[8] = {
        " p0", " p1", " p2", " p3",
        " p4", " p5", " p6", " p7"
    };
    if (argc > 8) argc = 8;

    for (size_t i = 0; i < argc; ++i)
    {
        char* name = params[i];
        if (!rplni_state_add_name(state, name)) return 0;

        rplni_id_t id = rplni_state_id_by_name(state, name);
        rplni_func_add_param(v.value._func, id);
    }
#define prog (v.value._func->prog)
    for (size_t i = 0; i < argc; ++i)
    {
        CODE_I(RPLNI_OP_LOAD, i);
    }
    CODE_I(op, 0);
#undef prog
    rplni_scope_add_var(scope, id);
    rplni_scope_store_var(scope, id, &v);
    rplni_value_clean(&v);

    return 1;
}


#ifndef NDEBUG
void rplni_dump_leaked()
{
    LOG("leaked:\n");

    for (size_t i = 0; i < all_objs->size; ++i)
    {
        LOG("  %p\n", all_objs->values.any[i]);
    }
}
#endif
