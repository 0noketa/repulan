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

    str->refs = 1;
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
int rplni_str_ref(struct rplni_str *str)
{
    if (str == NULL) return 0;
    if (str->refs == RPLNI_REFS_MAX) return 0;

    str->refs++;

    return 1;
}
int rplni_str_unref(struct rplni_str* str, struct rplni_state* state)
{
    if (str == NULL) return 0;
    if (state != NULL && rplni_ptrlist_has(state->deallocation_history, str)) return 1;

    str->refs--;

    if (str->refs > 0) return 1;
    if (state == NULL)
    {
        state = str->owner;
    }

    rplni_ptrlist_add(state->deallocation_history, str);

    LOG("del str: %p [%s]\n", str, str->value);

    rplni_state_free(str->owner, str->value);
    rplni_state_free(str->owner, str);

    return 1;
}
int rplni_str_del(struct rplni_str* str, struct rplni_state* state)
{
    if (str == NULL) return 0;
    if (state != NULL && rplni_ptrlist_has(state->deallocation_history, str)) return 1;

    str->refs--;

    if (str->refs > 0) return 1;

    assert (state != NULL);
    rplni_ptrlist_add(state->deallocation_history, str);

    rplni_state_free(str->owner, str->value);
    rplni_state_free(str->owner, str);

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

    list->refs = 1;
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

    list->refs = 1;
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
int rplni_list_ref(struct rplni_list *list)
{
    if (list == NULL) return 0;
    if (list->refs == RPLNI_REFS_MAX) return 0;

    list->refs++;

    return 1;
}
int rplni_list_unref(struct rplni_list* list, struct rplni_state* state)
{
    if (list == NULL) return 0;
    if (state != NULL && rplni_ptrlist_has(state->deallocation_history, list)) return 1;

    list->refs--;

    if (list->refs > rplni_list_count_circular_refs(list, NULL, NULL)) return 1;

    list->refs = 0;

    if (state == NULL)
    {
        state = list->owner;
        rplni_ptrlist_clear(state->deallocation_history);
    }

    rplni_ptrlist_add(state->deallocation_history, list);

    for (size_t i = 0; i < list->size; ++i)
    {
        rplni_value_clean(list->values + i, state);
    }

    LOG("del unrefed list: %p,%p[%u]\n", list, list->values, (int)list->size);

    rplni_state_free(list->owner, list->values);
    rplni_state_free(list->owner, list);

    return 1;
}
int rplni_list_del(struct rplni_list* list, struct rplni_state* state)
{
    if (list == NULL) return 0;

    LOG("del list: %p,%p[%u]\n", list, list->values, (int)list->size);

    rplni_state_free(list->owner, list->values);
    rplni_state_free(list->owner, list);

    return 1;
}
int rplni_list_count_circular_refs(struct rplni_list* list, void* root, struct rplni_ptrlist* known_nodes)
{
    if (list == NULL) return 0;

    assert((root == NULL) == (known_nodes == NULL));

    int is_root = known_nodes == NULL;
    if (is_root)
    {
        root = list;
        known_nodes = rplni_ptrlist_new();
        rplni_ptrlist_add(known_nodes, list);
    }
    else if (list == root)
    {
        return 1;
    }
    else if (rplni_ptrlist_has(known_nodes, list))
    {
        return 0;
    }
    else if (list->refs > 1)
    {
        return 0;
    }

    int result = 0;
    for (size_t i = 0; i < list->size; ++i)
    {
        struct rplni_value* value = list->values + i;

        result += rplni_value_count_circular_refs(value, root, known_nodes);
    }

    if (is_root)
    {
        rplni_ptrlist_del(known_nodes);
    }

    return result;
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

    if (!rplni_value_ref(value)) return 0;

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

    struct rplni_ptrlist* params = rplni_ptrlist_new_as_strlist();
    if (params == NULL) return 0;

    struct rplni_ptrlist* members = rplni_ptrlist_new_as_strlist();
    if (members == NULL)
    {
        rplni_ptrlist_del(params);
        return 0;
    }

    if (!rplni_prog_init(&func->prog, state))
    {
        rplni_ptrlist_del(params);
        rplni_ptrlist_del(members);
        return 0;
    }


    func->refs = 1;
    func->owner = state;
    func->type = type;
    func->params = params;
    func->members = members;
    return 1;
}
int rplni_func_ref(struct rplni_func* func)
{
    if (func == NULL) return 0;
    if (func->refs >= RPLNI_REFS_MAX) return 0;

    func->refs++;

    return 1;
}
int rplni_func_unref(struct rplni_func* func, struct rplni_state* state)
{
    if (func == NULL) return 0;
    if (state != NULL && rplni_ptrlist_has(state->deallocation_history, func)) return 1;

    func->refs--;

    if (func->refs > rplni_func_count_circular_refs(func, NULL, NULL)) return 1;

    func->refs = 0;

    if (state == NULL)
    {
        state = func->owner;
        rplni_ptrlist_clear(state->deallocation_history);
    }
    rplni_ptrlist_add(state->deallocation_history, func);

    rplni_prog_clean(&(func->prog), func->owner);

    rplni_ptrlist_del(func->params);
    rplni_ptrlist_del(func->members);
    rplni_state_free(func->owner, func);

    return 1;
}
int rplni_func_del(struct rplni_func* func, struct rplni_state* state)
{
    if (func == NULL) return 0;

    rplni_prog_del(&(func->prog), func->owner);

    rplni_ptrlist_del(func->params);
    rplni_ptrlist_del(func->members);
    rplni_state_free(state, func);

    return 1;
}
int rplni_func_count_circular_refs(struct rplni_func* func, void* root, struct rplni_ptrlist* known_nodes)
{
    if (func == NULL) return 0;

    assert((root == NULL) == (known_nodes == NULL));

    int is_root = known_nodes == NULL;
    if (is_root)
    {
        root = func;
        known_nodes = rplni_ptrlist_new();
        rplni_ptrlist_add(known_nodes, func);
    }
    else if (func == root)
    {
        return 1;
    }
    else if (rplni_ptrlist_has(known_nodes, func))
    {
        return 0;
    }
    else if (func->refs > 1)
    {
        return 0;
    }

    int result = 0;
    for (size_t i = 0; i < func->prog.size; ++i)
    {
        struct rplni_cmd* cmd = func->prog.code + i;

        result += rplni_value_count_circular_refs(&cmd->arg, root, known_nodes);
    }

    if (is_root)
    {
        rplni_ptrlist_del(known_nodes);
    }

    return result;
}
int rplni_func_add_param(struct rplni_func* func, char* name, int copy_str)
{
    if (func == NULL || name == NULL) return 0;

    return rplni_ptrlist_push_str(func->params, name, copy_str);
}
int rplni_func_run(struct rplni_func* func, struct rplni_state* state, struct rplni_scope *scope)
{
    if (func == NULL || state == NULL) return 0;

    struct rplni_scope tmp_scope;
    int use_tmp_scope = scope == NULL;
    if (use_tmp_scope)
    {
        if (!rplni_scope_init(&tmp_scope, state)) return 0;
        scope = &tmp_scope;
    }

    size_t n_params = func->params->size;
    if (state->data_stack->size < n_params) return 0;

    LOG("begin func/%d call (stack: %d)\n", (int)n_params, (int)state->data_stack->size);

    if (use_tmp_scope)
    {
        for (size_t i = 0; i < n_params; ++i)
        {
            char* name = func->params->values.cstr[i];

            rplni_scope_add_var(scope, name);
        }
    }
    else
    {
        struct rplni_tmpstr *tmpstr = rplni_tmpstr_new();
        rplni_tmpstr_add_cstr(tmpstr, 1, "{");
        for (size_t i = 0; i < scope->size; ++i)
        {
            struct rplni_named* var = scope->vars + i;
            struct rplni_value* val = &var->value;

            rplni_tmpstr_add_cstr(tmpstr, strlen(var->name), var->name);
            rplni_tmpstr_add_cstr(tmpstr, 1, ":");
            rplni_tmpstr_add_value(tmpstr, val, NULL);
            rplni_tmpstr_add_cstr(tmpstr, 1, ",");
        }
        rplni_tmpstr_add_cstr(tmpstr, 1, "}");

        LOG("%s\n", tmpstr->s);

        rplni_tmpstr_del(tmpstr);
    }

    struct rplni_value tmp;
    rplni_value_init(&tmp);

    for (size_t i = n_params; i-- > 0;)
    {
        rplni_list_pop(state->data_stack, &tmp);
        rplni_scope_store_var_by_index(scope, i, &tmp);
        
        rplni_value_clean(&tmp, state);
    }

    for (size_t i = 0; i < scope->size; ++i)
    {
        struct rplni_named* v = scope->vars + i;
        0;
        LOG("");
    }

    rplni_state_push_scope(state, scope);
    rplni_prog_run(&(func->prog), state, func->params->size);
    rplni_state_pop_scope(state, NULL);
    if (use_tmp_scope)
    {
        rplni_scope_clean(scope, state);
    }

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

    for (size_t i = 0; i < funcdef->params->size; ++i)
    {
        char* name = funcdef->params->values.cstr[i];
        rplni_scope_add_var(&closure->scope, name);
    }

    for (size_t i = funcdef->params->size; i < funcdef->members->size; ++i)
    {
        char* name = funcdef->members->values.cstr[i];
        rplni_scope_add_var(&closure->scope, name);

        size_t member_idx = rplni_scope_var_index(&closure->scope, name);

        struct rplni_scope* var_scope;
        struct rplni_value var_value;
        size_t var_idx;
        if (rplni_state_find_var(state, name, &var_scope, &var_idx))
        {
            rplni_scope_load_var_by_index(var_scope, var_idx, &var_value);
            /* copy at here */
        }
        else
        {
            rplni_value_init(&var_value);
        }

        rplni_scope_store_var_by_index(&closure->scope, member_idx, &var_value);
        rplni_value_clean(&var_value, NULL);
    }

    rplni_func_ref(funcdef);
    closure->funcdef = funcdef;
    closure->owner = state;
    closure->refs = 1;
    return 1;
}
int rplni_closure_ref(struct rplni_closure* closure)
{
    if (closure == NULL) return 0;
    if (closure->refs >= RPLNI_REFS_MAX) return 0;

    closure->refs++;
    return 1;
}
int rplni_closure_unref(struct rplni_closure* closure, struct rplni_state *state)
{
    if (closure == NULL) return 0;
    if (state != NULL && rplni_ptrlist_has(state->deallocation_history, closure)) return 1;

    closure->refs--;

    if (closure->refs > rplni_closure_count_circular_refs(closure, NULL, NULL)) return 1;
    if (state == NULL)
    {
        state = closure->owner;
        rplni_ptrlist_clear(state->deallocation_history);
    }
    rplni_ptrlist_add(state->deallocation_history, closure);

    LOG("del unrefed closure\n");
    rplni_func_unref(closure->funcdef, state);
    rplni_scope_clean(&closure->scope, state);
    rplni_state_free(closure->owner, closure);
    LOG("end del unrefed closure\n");
    return 1;
}
int rplni_closure_del(struct rplni_closure* closure, struct rplni_state* state)
{
    if (closure == NULL) return 0;

    LOG("del closure\n");
    rplni_func_unref(closure->funcdef, state);
    rplni_scope_clean(&closure->scope, state);
    rplni_state_free(closure->owner, closure);
    LOG("end del closure\n");
    return 1;
}
int rplni_closure_count_circular_refs(struct rplni_closure* closure, void* root, struct rplni_ptrlist* known_nodes)
{
    if (closure == NULL) return 0;

    assert((root == NULL) == (known_nodes == NULL));

    int is_root = known_nodes == NULL;
    if (is_root)
    {
        root = closure;
        known_nodes = rplni_ptrlist_new();
        rplni_ptrlist_add(known_nodes, closure);
    }
    else if (closure == root)
    {
        return 1;
    }
    else if (rplni_ptrlist_has(known_nodes, closure))
    {
        return 0;
    }
    else if (closure->refs > 1)
    {
        return 0;
    }

    int result = rplni_func_count_circular_refs(closure->funcdef, root, known_nodes);

    for (size_t i = 0; i < closure->scope.size; ++i)
    {
        struct rplni_named* named = closure->scope.vars + i;

        result += rplni_value_count_circular_refs(&named->value, root, known_nodes);
    }

    if (is_root)
    {
        rplni_ptrlist_del(known_nodes);
    }

    return result;
}
int rplni_closure_run(struct rplni_closure* closure, struct rplni_state* state)
{
    if (closure == NULL || state == NULL) return 0;

    rplni_func_run(closure->funcdef, state, &(closure->scope));
    return 1;
}
