#include "rplni.h"

#ifndef NDEBUG
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(...)
#endif


struct rplni_str *rplni_str_new(size_t size, const char *value, struct rplni_state* state)
{
    if (value == NULL) return NULL;

    struct rplni_str *str = rplni_state_malloc(state, sizeof(struct  rplni_str));

    if (str == NULL) return NULL;

    if (rplni_str_init(str, size, value, state))
    {
        LOG("new str: %p [%s]\n", str, str->value);
        return str;
    }

    rplni_state_free(state, str);

    return NULL;
}
int rplni_str_init(struct rplni_str *str, size_t size, const char *value, struct rplni_state* state)
{
    if (str == NULL) return 0;

    str->owner = state;
    str->size = size;
    str->value = rplni_state_strdup(state, size, value);

    if (str->value == NULL)
    {
        str->size = 0;

        return 0;
    }

    return 1;
}
int rplni_str_find_living_objs(struct rplni_str* str, struct rplni_ptrlist* known_nodes)
{
    if (str == NULL || known_nodes == NULL) return 0;
    if (rplni_ptrlist_has(known_nodes, str)) return 1;

    rplni_ptrlist_add(known_nodes, str, NULL);
    rplni_ptrlist_add(known_nodes, str->value, NULL);

    return 1;
}
int rplni_str_add(struct rplni_str* str, const struct rplni_str* str2)
{
    if (str == NULL || str2 == NULL) return 0;
    if (str2->size == 0) return 1;

    char* s = rplni_state_realloc(str->owner, str->value, str->size + str2->size + 1);

    if (s == NULL) return 0;

    str->value = s;
    strncpy(str->value + str->size, str2->value, str2->size);
    str->size += str2->size;
    str->value[str->size] = 0;

    return 1;
}
int rplni_str_add_cstr(struct rplni_str* str, size_t size, const char* str2)
{
    if (str == NULL || str2 == NULL) return 0;
    if (size == 0) return 1;

    char* s = rplni_state_realloc(str->owner, str->value, str->size + size + 1);

    if (s == NULL) return 0;

    str->value = s;
    strncpy(str->value + str->size, str2, size);
    str->size += size;
    str->value[str->size] = 0;

    return 1;
}

struct rplni_list *rplni_list_new(size_t cap, struct rplni_state* state)
{
    struct rplni_list *list = rplni_state_malloc(state, sizeof(struct rplni_list));

    if (list == NULL) return NULL;

    if (rplni_list_init(list, cap, state))
    {
        LOG("new list: %p %p @ %p\n", list, list->values, state);
        return list;
    }

    rplni_state_free(state, list);

    return NULL;
}
int rplni_list_init(struct rplni_list *list, size_t cap, struct rplni_state* state)
{
    if (list == NULL) return 0;

    list->owner = state;
    list->cap = RPLNI_LIST_CEILED_SIZE(cap);
    list->size = 0;
    list->values = rplni_state_malloc(state, list->cap * sizeof(struct rplni_value));

    if (list->values == NULL)
    {
        list->cap = 0;
        list->size = 0;

        return 0;
    }

    rplni_values_init(list->cap, list->values);

    return 1;
}
struct rplni_list* rplni_list_new_with_captured(size_t size, struct rplni_list *stack, struct rplni_state* state)
{
    struct rplni_list* list = rplni_state_malloc(state, sizeof(struct rplni_list));

    if (list == NULL) return NULL;

    if (rplni_list_init_with_captured(list, size, stack, state))
    {
        LOG("new list: %p %p @ %p\n", list, list->values, state);
        return list;
    }

    rplni_state_free(state, list);

    return NULL;
}
int rplni_list_init_with_captured(struct rplni_list* list, size_t size, struct rplni_list *stack, struct rplni_state* state)
{
    if (list == NULL || stack == NULL || stack->size < size) return 0;

    list->owner = state;
    list->cap = RPLNI_LIST_CEILED_SIZE(size);
    list->size = size;
    list->values = rplni_state_malloc(state, list->cap * sizeof(struct rplni_value));

    if (list->values == NULL)
    {
        list->cap = 0;
        list->size = 0;

        return 0;
    }

    memcpy(list->values, stack->values + stack->size - size, size * sizeof(struct rplni_value));
    stack->size -= size;

    return 1;
}
int rplni_list_find_living_objs(struct rplni_list* list, struct rplni_ptrlist* known_nodes)
{
    if (list == NULL || known_nodes == NULL) return 0;
    if (rplni_ptrlist_has(known_nodes, list)) return 1;

    /* ignore global stack */
    if (list->owner != NULL)
    {
        rplni_ptrlist_add(known_nodes, list, NULL);
        rplni_ptrlist_add(known_nodes, list->values, NULL);
    }

    for (size_t i = 0; i < list->size; ++i)
    {
        struct rplni_value* value = list->values + i;

        rplni_value_find_living_objs(value, known_nodes);
    }

    return 1;
}
int rplni_list_push(struct rplni_list *list, struct rplni_value *value)
{
    if (list == NULL || value == NULL) return 0;

    if (list->cap - list->size < RPLNI_LIST_BLOCK_SIZE / 2)
    {
        size_t new_cap = list->cap + RPLNI_LIST_BLOCK_SIZE;
        struct rplni_value *values = rplni_state_realloc(list->owner, list->values, new_cap * sizeof(struct rplni_value));

        if (values == NULL) return 0;

        list->cap = new_cap;
        list->values = values;
    }

    list->values[list->size++] = *value;
    return 1;
}
int rplni_list_pop(struct rplni_list *list, struct rplni_value *out_value)
{
    if (list == NULL || list->size == 0) return 0;

    struct rplni_value value = list->values[list->size - 1];

    list->size--;
    if (out_value != NULL) *out_value = value;

    return 1;
}

struct rplni_func* rplni_func_new(enum rplni_func_type type, struct rplni_state* state)
{
    struct rplni_func* func = rplni_state_malloc(state, sizeof(struct rplni_func));

    if (func == NULL) return NULL;
    if (!rplni_func_init(func, type, state)) return NULL;

    LOG("new func: %p @ %p\n", func, state);

    return func;
}
int rplni_func_init(struct rplni_func* func, enum rplni_func_type type, struct rplni_state* state)
{
    if (func == NULL) return 0;

    struct rplni_ptrlist* params = rplni_ptrlist_new(1, state);
    if (params == NULL) return 0;

    struct rplni_ptrlist* members = rplni_ptrlist_new(1, state);
    if (members == NULL) return 0;

    if (!rplni_prog_init(&func->prog, state)) return 0;

    func->owner = state;
    func->type = type;
    func->params = params;
    func->members = members;
    return 1;
}
int rplni_func_find_living_objs(struct rplni_func* func, struct rplni_ptrlist* known_nodes)
{
    if (func == NULL || known_nodes == NULL) return 0;
    if (rplni_ptrlist_has(known_nodes, func)) return 1;

    rplni_ptrlist_add(known_nodes, func, NULL);
    rplni_ptrlist_add(known_nodes, func->members, NULL);
    rplni_ptrlist_add(known_nodes, func->members->values.any, NULL);
    rplni_ptrlist_add(known_nodes, func->params, NULL);
    rplni_ptrlist_add(known_nodes, func->params->values.any, NULL);
    rplni_ptrlist_add(known_nodes, func->prog.code, NULL);

    for (size_t i = 0; i < func->prog.size; ++i)
    {
        struct rplni_cmd* cmd = func->prog.code + i;

        rplni_value_find_living_objs(&cmd->arg, known_nodes);
    }

    return 1;
}
int rplni_func_add_param(struct rplni_func* func, rplni_id_t id)
{
    if (func == NULL) return 0;

    return rplni_ptrlist_push_uint(func->params, id, func->owner);
}
int rplni_func_run(struct rplni_func* func, struct rplni_state* state)
{
    if (func == NULL || state == NULL) return 0;

    struct rplni_scope scope;
    if (!rplni_scope_init(&scope, state)) return 0;

    size_t n_params = func->params->size;
    if (state->data_stack->size < n_params) return 0;

    LOG("begin func/%d call (stack: %d)\n", (int)n_params, (int)state->data_stack->size);

    for (size_t i = 0; i < n_params; ++i)
    {
        rplni_id_t id = func->params->values._uint[i];

        rplni_scope_add_var(&scope, id);
    }
    struct rplni_value tmp;
    rplni_value_init(&tmp);

    for (size_t i = n_params; i-- > 0;)
    {
        rplni_list_pop(state->data_stack, &tmp);
        rplni_scope_store_var_by_index(&scope, i, &tmp);
        
        rplni_value_clean(&tmp);
    }

    rplni_state_push_scope(state, &scope);
    {
        struct rplni_scope* outer_scope;
        rplni_state_outer_scope(state, &outer_scope);
        struct rplni_tmpstr* tmpstr = rplni_tmpstr_new();
        struct rplni_scope* scopes[2] = { outer_scope , &scope };
        for (int i = 0; i < 2; ++i)
        {

            rplni_tmpstr_add_cstr(tmpstr, 1, "{");
            for (size_t j = 0; j < scopes[i]->size; ++j)
            {
                struct rplni_named* var = scopes[i]->vars + j;
                struct rplni_value* val = &var->value;
                const char* name;
                rplni_state_name_by_id(state, var->id, &name);

                rplni_tmpstr_add_cstr(tmpstr, strlen(name), name);
                rplni_tmpstr_add_cstr(tmpstr, 1, ":");
                rplni_tmpstr_add_value(tmpstr, val, NULL);
                rplni_tmpstr_add_cstr(tmpstr, 1, ",");
            }
            rplni_tmpstr_add_cstr(tmpstr, 1, "}");

        }
        LOG("%s\n", tmpstr->s);

        rplni_tmpstr_del(tmpstr);
    }


    rplni_prog_run(&(func->prog), state, func->params->size);
    rplni_state_pop_scope(state, NULL);
    rplni_scope_clean(&scope);
    
    LOG("end func call\n");

    return 1;
}


struct rplni_closure* rplni_closure_new(struct rplni_func* funcdef, struct rplni_state* state)
{
    if (funcdef == NULL || state == NULL) return NULL;

    struct rplni_closure* closure = rplni_state_malloc(state, sizeof(struct rplni_closure));
    if (closure == NULL) return NULL;

    if (!rplni_closure_init(closure, funcdef, state))
    {
        rplni_state_free(state, closure);
        return 0;
    }

    return closure;
}
int rplni_closure_init(struct rplni_closure* closure, struct rplni_func* funcdef, struct rplni_state* state)
{
    if (closure == NULL || funcdef == NULL || state == NULL) return 0;

    if (!rplni_scope_init(&closure->scope, state)) return 0;

    for (size_t i = 0; i < funcdef->members->size; ++i)
    {
        rplni_id_t id = funcdef->members->values._uint[i];
        rplni_scope_add_var(&closure->scope, id);

        size_t member_idx = rplni_scope_var_index(&closure->scope, id);

        struct rplni_scope* var_scope;
        struct rplni_value var_value;
        size_t var_idx;
        if (rplni_state_find_var(state, id, &var_scope, &var_idx))
        {
            rplni_scope_load_var_by_index(var_scope, var_idx, &var_value);
            /* copy at here */
        }
        else
        {
            rplni_value_init(&var_value);
        }

        rplni_scope_store_var_by_index(&closure->scope, member_idx, &var_value);
        rplni_value_clean(&var_value);
    }

    closure->funcdef = funcdef;
    closure->owner = state;
    return 1;
}
int rplni_closure_find_living_objs(struct rplni_closure* closure, struct rplni_ptrlist* known_nodes)
{
    if (closure == NULL || known_nodes == NULL) return 0;
    if (rplni_ptrlist_has(known_nodes, closure)) return 1;

    rplni_ptrlist_add(known_nodes, closure, NULL);

    rplni_scope_find_living_objs(&closure->scope, known_nodes);
    rplni_func_find_living_objs(closure->funcdef, known_nodes);
    return 1;
}
int rplni_closure_run(struct rplni_closure* closure, struct rplni_state* state)
{
    if (closure == NULL || state == NULL) return 0;

    LOG("closure has scope with %d members\n", (int)closure->scope.size);
    rplni_state_push_scope(state, &(closure->scope));
    int r = rplni_func_run(closure->funcdef, state);
    rplni_state_pop_scope(state, NULL);
    return r;
}
