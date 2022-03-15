#include "rplni.h"


struct rplni_str *rplni_str_new(char *value, struct rplni_scope* scope)
{
    if (value == NULL) return NULL;

    struct rplni_str *str = rplni_scope_malloc(scope, sizeof(struct  rplni_str));

    if (str == NULL) return NULL;

    if (rplni_str_init(str, value, scope))
    {
        fprintf(stderr, "new str: %p [%s]\n", str, str->value);
        rplni_scope_add_str(scope, str);

        return str;
    }

    rplni_scope_free(scope, str);

    return NULL;
}
int rplni_str_init(struct rplni_str *str, char *value, struct rplni_scope* scope)
{
    if (str == NULL) return 0;

    str->refs = 1;
    str->owner = scope;
    str->size = strlen(value);
    str->value = rplni_scope_strdup(scope, value);

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
int rplni_str_unref(struct rplni_str* str, struct rplni_scope* scope)
{
    if (str == NULL) return 0;
    if (scope != NULL && rplni_ptrlist_has(scope->deallocation_history, str)) return 1;

    str->refs--;

    if (str->refs > 0) return 1;
    if (scope == NULL)
    {
        scope = str->owner;
    }

    rplni_ptrlist_add(scope->deallocation_history, str);

    fprintf(stderr, "del str: %p [%s]\n", str, str->value);

    rplni_scope_remove_str(str->owner, str);
    rplni_scope_free(str->owner, str->value);
    rplni_scope_free(str->owner, str);

    return 1;
}
int rplni_str_del(struct rplni_str* str, struct rplni_scope* scope)
{
    if (str == NULL) return 0;
    if (scope != NULL && rplni_ptrlist_has(scope->deallocation_history, str)) return 1;

    str->refs--;

    if (str->refs > 0 && scope != NULL && !rplni_scope_has(scope, str)) return 1;
    if (scope == NULL)
    {
        scope = str->owner;
    }

    rplni_ptrlist_add(scope->deallocation_history, str);

    rplni_scope_remove_str(scope, str);
    rplni_scope_free(scope, str->value);
    rplni_scope_free(scope, str);

    return 1;
}
int rplni_scope_export_str(struct rplni_scope* scope, struct rplni_str* str, struct rplni_scope* scope2)
{
    if (str == NULL) return 0;
    if (scope != NULL && !rplni_scope_has(scope, str)) return 1;
    if (scope == NULL)
    {
        scope = str->owner;
    }

    fprintf(stderr, "export str %p (%p -> %p)\n", str, scope, scope2);

    rplni_scope_add(scope2, str);
    rplni_scope_add(scope2, str->value);
    rplni_scope_add_str(scope2, str);

    rplni_scope_remove(scope, str);
    rplni_scope_remove(scope, str->value);
    rplni_scope_remove_str(scope, str);

    str->owner = scope2;

    return rplni_str_ref(str);
}
int rplni_str_add(struct rplni_str* str, struct rplni_str* str2)
{
    if (str == NULL || str2 == NULL) return 0;
    if (str2->size == 0) return 1;

    struct rplni_scope* scope = str->owner;
    char* s = rplni_scope_realloc(scope, str->value, str->size + str2->size + 1);

    if (s == NULL) return 0;

    str->value = s;
    strcpy(str->value + str->size, str2->value);

    str->size += str2->size;

    return 1;
}
int rplni_str_add_cstr(struct rplni_str* str, char* str2)
{
    if (str == NULL || str2 == NULL) return 0;

    size_t size2 = strlen(str2);
    if (size2 == 0) return 1;

    struct rplni_scope* scope = str->owner;
    char* s = rplni_scope_realloc(scope, str->value, str->size + size2 + 1);

    if (s == NULL) return 0;

    str->value = s;
    strcpy(str->value + str->size, str2);

    str->size += size2;

    return 1;
}

struct rplni_list *rplni_list_new(size_t cap, struct rplni_scope* scope)
{
    struct rplni_list *list = rplni_scope_malloc(scope, sizeof(struct rplni_list));

    if (list == NULL) return NULL;

    if (rplni_list_init(list, cap, scope))
    {
        fprintf(stderr, "new list: %p\n", list);
        rplni_scope_add_list(scope, list);
        return list;
    }

    rplni_scope_free(scope, list);

    return NULL;
}
int rplni_list_init(struct rplni_list *list, size_t cap, struct rplni_scope* scope)
{
    if (list == NULL) return 0;

    list->refs = 1;
    list->owner = scope;
    list->cap = RPLNI_LIST_CEILED_SIZE(cap);
    list->size = 0;
    list->values = rplni_scope_malloc(scope, list->cap * sizeof(struct rplni_value));

    if (list->values == NULL)
    {
        list->cap = 0;
        list->size = 0;

        return 0;
    }

    rplni_values_init(list->cap, list->values);

    return 1;
}
struct rplni_list* rplni_list_new_with_captured(size_t size, struct rplni_list *stack, struct rplni_scope* scope)
{
    struct rplni_list* list = rplni_scope_malloc(scope, sizeof(struct rplni_list));

    if (list == NULL) return NULL;

    if (rplni_list_init_with_captured(list, size, stack, scope))
    {
        fprintf(stderr, "new list: %p\n", list);
        rplni_scope_add_list(scope, list);
        return list;
    }

    rplni_scope_free(scope, list);

    return NULL;
}
int rplni_list_init_with_captured(struct rplni_list* list, size_t size, struct rplni_list *stack, struct rplni_scope* scope)
{
    if (list == NULL || stack == NULL || stack->size < size) return 0;

    list->refs = 1;
    list->owner = scope;
    list->cap = RPLNI_LIST_CEILED_SIZE(size);
    list->size = size;
    list->values = rplni_scope_malloc(scope, list->cap * sizeof(struct rplni_value));

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
int rplni_list_unref(struct rplni_list* list, struct rplni_scope* scope)
{
    if (list == NULL) return 0;
    if (scope != NULL && rplni_ptrlist_has(scope->deallocation_history, list)) return 1;

    list->refs--;

    if (list->refs > 0) return 1;
    if (scope == NULL)
    {
        scope = list->owner;
        rplni_ptrlist_clear(scope->deallocation_history);
    }

    rplni_ptrlist_add(scope->deallocation_history, list);

    for (size_t i = 0; i < list->size; ++i)
    {
        rplni_value_clean(list->values + i, scope);
    }

    fprintf(stderr, "del list: %p[%u]\n", list, (int)list->size);

    rplni_scope_remove_list(list->owner, list);
    rplni_scope_free(list->owner, list->values);
    rplni_scope_free(list->owner, list);

    return 1;
}
int rplni_list_del(struct rplni_list* list, struct rplni_scope* scope)
{
    if (list == NULL) return 0;
    if (scope != NULL && rplni_ptrlist_has(scope->deallocation_history, list)) {
        fprintf(stderr, "failed: del %p @ %p was deleted already", list, scope);
        return 1;
    }
    list->refs--;

    if (list->refs > 0 && scope != NULL && !rplni_scope_has(scope, list)) {
        fprintf(stderr, "failed: del %p not in %p", list, scope);
        return 1;
    }

    if (scope == NULL)
    {
        scope = list->owner;
        rplni_ptrlist_clear(scope->deallocation_history);
    }

    rplni_ptrlist_add(scope->deallocation_history, list);

    for (size_t i = 0; i < list->size; ++i)
    {
        rplni_value_del(list->values + i, scope);
    }

    rplni_scope_remove_list(scope, list);
    rplni_scope_free(scope, list->values);
    rplni_scope_free(scope, list);

    return 1;
}
int rplni_scope_export_list(struct rplni_scope* scope, struct rplni_list* list, struct rplni_scope* scope2)
{
    if (list == NULL || scope2 == NULL) return 0;
    if (scope != NULL && !rplni_scope_has(scope, list)) return 1;
    if (scope == NULL)
    {
        scope = list->owner;
    }

    fprintf(stderr, "export list %p (%p -> %p)\n", list, scope, scope2);

    rplni_scope_add(scope2, list);
    rplni_scope_add(scope2, list->values);
    rplni_scope_add_list(scope2, list);

    rplni_scope_remove(scope, list);
    rplni_scope_remove(scope, list->values);
    rplni_scope_remove_list(scope, list);

    for (size_t i = 0; i < list->size; ++i)
    {
        rplni_scope_export_value(scope, list->values + i, scope2);
    }

    list->owner = scope2;

    return rplni_list_ref(list);
}
int rplni_list_push(struct rplni_list *list, struct rplni_value *value)
{
    if (list == NULL || value == NULL) return 0;

    if (list->cap - list->size < RPLNI_LIST_BLOCK_SIZE / 2)
    {
        size_t new_cap = list->cap + RPLNI_LIST_BLOCK_SIZE;
        struct rplni_value *values = rplni_scope_realloc(list->owner, list->values, new_cap * sizeof(struct rplni_value));

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

    //if (list->cap - list->size > RPLNI_LIST_BLOCK_SIZE * 2)
    //{
    //    size_t new_cap = list->cap - RPLNI_LIST_BLOCK_SIZE;
    //    struct rplni_value *values = rplni_scope_realloc(list->owner, list->values, new_cap * sizeof(struct rplni_value));

    //    if (values == NULL) return 0;

    //    list->cap = new_cap;
    //    list->values = values;
    //}

    struct rplni_value value = list->values[list->size - 1];

    list->size--;
    if (out_value != NULL) *out_value = value;

    return 1;
}


struct rplni_func* rplni_func_new(enum rplni_func_type type, struct rplni_scope* scope)
{
    struct rplni_func* func = rplni_scope_malloc(scope, sizeof(struct rplni_func));

    if (func == NULL) return NULL;
    if (!rplni_func_init(func, type, scope)) return NULL;

    fprintf(stderr, "new func: %p\n", func);

    return func;
}
int rplni_func_init(struct rplni_func* func, enum rplni_func_type type, struct rplni_scope* scope)
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

    if (!rplni_prog_init(&func->prog, scope))
    {
        rplni_ptrlist_del(params);
        rplni_ptrlist_del(members);
        return 0;
    }


    func->refs = 1;
    func->owner = scope;
    func->type = type;
    func->params = params;
    func->members = members;
    rplni_ptrlist_add(scope->funcs, func);

    return 1;
}
int rplni_func_ref(struct rplni_func* func)
{
    if (func == NULL) return 0;
    if (func->refs >= RPLNI_REFS_MAX) return 0;

    func->refs++;

    return 1;
}
int rplni_func_unref(struct rplni_func* func, struct rplni_scope* scope)
{
    if (func == NULL) return 0;
    if (scope != NULL && rplni_ptrlist_has(scope->deallocation_history, func)) return 1;

    func->refs--;

    if (func->refs > 0) return 1;
    if (scope == NULL)
    {
        scope = func->owner;
        rplni_ptrlist_clear(scope->deallocation_history);
    }
    rplni_ptrlist_add(scope->deallocation_history, func);

    rplni_prog_clean(&(func->prog), func->owner, scope);

    rplni_ptrlist_del(func->params);
    rplni_scope_free(func->owner, func);

    return 1;
}
int rplni_func_del(struct rplni_func* func, struct rplni_scope* scope)
{
    if (func == NULL) return 0;
    if (scope != NULL && rplni_ptrlist_has(scope->deallocation_history, func)) return 1;

    func->refs--;

    if (func->refs > 0 && scope != NULL && !rplni_scope_has(scope, func)) return 1;
    if (scope == NULL)
    {
        scope = func->owner;
        rplni_ptrlist_clear(scope->deallocation_history);
    }
    rplni_ptrlist_add(scope->deallocation_history, func);

    rplni_prog_del(&(func->prog), func->owner, scope);

    rplni_ptrlist_del(func->params);
    rplni_scope_free(scope, func);

    return 1;
}
int rplni_scope_export_func(struct rplni_scope* scope, struct rplni_func* func, struct rplni_scope* scope2)
{
    if (func == NULL || scope2 == NULL) return 0;
    if (scope != NULL && !rplni_scope_has(scope, func)) return 1;
    if (scope == NULL)
    {
        scope = func->owner;
    }

    fprintf(stderr, "export func %p -> %p\n", &scope, &scope2);


    rplni_scope_export_prog(scope, &(func->prog), scope2);
    /* params are not managed by scopes */

    rplni_scope_add(scope2, func);
    rplni_scope_add_func(scope2, func);
    rplni_scope_remove(scope, func);
    rplni_scope_remove_func(scope, func);

    func->owner = scope2;

    return rplni_func_ref(func);
}
int rplni_func_add_param(struct rplni_func* func, char* name, int copy_str)
{
    if (func == NULL || name == NULL) return 0;

    return rplni_ptrlist_push_str(func->params, name, copy_str);
}
int rplni_func_run(struct rplni_func* func, struct rplni_state* state)
{
    if (func == NULL || state == NULL) return 0;

    struct rplni_scope scope;
    if (!rplni_scope_init(&scope)) return 0;

    size_t n_params = rplni_ptrlist_len(func->params);
    if (state->data_stack->size < n_params) return 0;

    struct rplni_value tmp;
    rplni_value_init(&tmp);

    fprintf(stderr, "begin func/%d call (stack: %d)\n", (int)n_params, (int)state->data_stack->size);

    for (size_t i = 0; i < n_params; ++i)
    {
        char* name = func->params->values.cstr[i];

        rplni_scope_add_var(&scope, name);
    }

    for (size_t i = n_params; i-- > 0;)
    {
        rplni_list_pop(state->data_stack, &tmp);
        rplni_scope_store_var_by_index(&scope, i, &tmp);
        
        rplni_value_clean(&tmp, &scope);
    }

    rplni_state_push_scope(state, &scope);
    rplni_prog_run(&(func->prog), state);
    rplni_state_pop_scope(state, NULL);

    fputs("end func call\n", stderr);

    rplni_scope_clean(&scope);

    return 1;
}

