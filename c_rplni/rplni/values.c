#include "rul0i.h"


int rul0i_value_init(struct rul0i_value *value)
{
    if (value == NULL) return 0;

    value->type = RUL0I_UINT;
    value->value._uint = 0;

    return 1;
}
int rul0i_value_clean(struct rul0i_value *value, struct rul0i_scope *scope)
{
    if (value == NULL) return 0;

    switch (value->type)
    {
        case RUL0I_UINT:
            return 1;
        case RUL0I_STR:
            if (!rul0i_str_unref(value->value._str, scope)) return 0;
            break;
        case RUL0I_LIST:
            if (!rul0i_list_unref(value->value._list, scope)) return 0;
            break;
        default:
            return 0;
    }

    value->type = RUL0I_UINT;
    value->value._uint = 0;

    return 1;
}
int rul0i_value_ref(struct rul0i_value* value)
{
    if (value == NULL) return 0;

    switch (value->type)
    {
    case RUL0I_UINT:
        return 1;
    case RUL0I_STR:
        if (!rul0i_str_ref(value->value._str)) return 0;
        break;
    case RUL0I_LIST:
        if (!rul0i_list_ref(value->value._list)) return 0;
        break;
    default:
        return 0;
    }

    return 1;
}
int rul0i_value_eq(struct rul0i_value* value, struct rul0i_value* value2)
{
    if (value == NULL || value2 == NULL || value->type != value2->type) return 0;

    return value->type == RUL0I_STR ? value->value._str == value2->value._str
        : value->type == RUL0I_LIST ? value->value._list == value2->value._list
        : value->value._uint == value2->value._uint;
}
int rul0i_value_del(struct rul0i_value* value, struct rul0i_scope* scope)
{
    if (scope == NULL || value == NULL) return 0;

    switch (value->type)
    {
    case RUL0I_UINT:
        return 1;
    case RUL0I_STR:
        if (!rul0i_str_del(value->value._str, scope)) return 0;
        break;
    case RUL0I_LIST:
        if (!rul0i_list_del(value->value._list, scope)) return 0;
        break;
    default:
        return 0;
    }

    value->type = RUL0I_UINT;
    value->value._uint = 0;

    return 1;
}
int rul0i_scope_export_value(struct rul0i_scope* scope, struct rul0i_value* value, struct rul0i_scope* scope2)
{
    if (scope == NULL || value == NULL) return 0;

    switch (value->type)
    {
    case RUL0I_UINT:
        return 1;
    case RUL0I_STR:
        if (!rul0i_scope_export_str(scope, value->value._str, scope2)) return 0;
        break;
    case RUL0I_LIST:
        if (!rul0i_scope_export_list(scope, value->value._list, scope2)) return 0;
        break;
    default:
        return 0;
    }

    return 1;
}

int rul0i_values_init(size_t size, struct rul0i_value* values)
{
    if (values == NULL) return 0;

    for (size_t i = 0; i < size; ++i)
    {
        rul0i_value_init(values + i);
    }

    return 1;
}
size_t rul0i_values_object_index(size_t size, struct rul0i_value* values, struct rul0i_value* value)
{
    if (values == NULL) return 0;
    if (value == NULL) return size;

    size_t idx = 0;
    for (; idx < size && values[idx].type != RUL0I_UINT && !rul0i_value_eq(values + idx, value); ++idx);

    return idx;
}
int rul0i_values_has_object(size_t size, struct rul0i_value* values, struct rul0i_value* value)
{
    if (values == NULL || value == NULL || value->type == RUL0I_UINT) return 0;

    size_t idx = rul0i_values_object_index(size, values, value);
    return idx < size;
}
int rul0i_values_add_object(
        size_t size, struct rul0i_value* values,
        struct rul0i_value* value,
        struct rul0i_scope* scope,
        size_t *out_size, struct rul0i_value* out_values)
{
    if (values == NULL) return 0;

    size_t idx = rul0i_values_object_index(size, values, value);
    if (idx >= size)
    {
        size_t size2 = size + RUL0I_LIST_BLOCK_SIZE;
        struct rul0i_value* values2 = rul0i_scope_realloc(scope, values, size2);

        if (values2 == NULL) return 0;

        rul0i_values_init(size2 - size, values + size);

        values = values2;
        size = size2;
    }

    values[idx] = *value;

    *out_size = size;
    *out_values = *values;

    return 1;
}
int rul0i_values_remove_object(size_t size, struct rul0i_value* values, struct rul0i_value* value)
{
    if (values == NULL) return 0;

    size_t idx = rul0i_values_object_index(size, values, value);
    if (idx >= size) return 0;

    rul0i_value_init(values + idx);

    return 1;
}


int rul0i_named_init(struct rul0i_named *named, char *name, struct rul0i_scope* scope)
{
    if (named == NULL || name == NULL) return 0;

    named->name = rul0i_scope_malloc(scope, strlen(name) + 1);

    rul0i_value_init(&(named->value));

    if (named->name != NULL)
    {
        strcpy(named->name, name);

        return 1;
    }

    return 0;
}
int rul0i_named_clean(struct rul0i_named *named, struct rul0i_scope *scope)
{
    if (named == NULL) return 0;

    rul0i_value_clean(&(named->value), scope);

    rul0i_scope_free(scope, named->name);
    named->name = NULL;

    return 1;
}

int rul0i_scope_init(struct rul0i_scope *scope)
{
    if (scope == NULL) return 0;

    scope->size = 0;
    scope->objs = rul0i_set_new();
    scope->strs = rul0i_set_new();
    scope->lists = rul0i_set_new();
    scope->vars = rul0i_scope_malloc(scope, 0);

    return scope->vars != NULL;
}
int rul0i_scope_clean(struct rul0i_scope *scope)
{
    if (scope == NULL) return 0;

    for (size_t i = 0; i < scope->size; ++i)
    {
        rul0i_named_clean(scope->vars + i, scope);
    }
    rul0i_scope_free(scope, scope->vars);
    scope->size = 0;
    scope->vars = NULL;

    printf("clean scope %p\n", scope);
    
    for (size_t i = 0; i < scope->lists->size; ++i)
    {
        struct rul0i_list* obj = scope->lists->values[i];
        if (obj == NULL) continue;

        printf("-list%p\n", obj);
        rul0i_list_del(obj, scope);
    }
    rul0i_set_del(scope->lists);
    scope->lists = NULL;

    for (size_t i = 0; i < scope->strs->size; ++i)
    {
        struct rul0i_list* obj = scope->strs->values[i];
        if (obj == NULL) continue;

        printf("-str%p\n", obj);
        rul0i_str_del(obj, scope);
    }
    rul0i_set_del(scope->strs);
    scope->strs = NULL;

    for (size_t i = 0; i < scope->objs->size; ++i)
    {
        void *p = scope->objs->values[i];
        if (p == NULL) continue;

        printf("-%p\n", p);
        rul0i_scope_free(scope, p);
    }
    rul0i_set_del(scope->objs);
    scope->objs = NULL;

    return 1;
}
void *rul0i_scope_malloc(struct rul0i_scope* scope, size_t size)
{
    void *p = malloc(size);

    if (scope == NULL) return p;

    if (rul0i_set_add(scope->objs, p)) return p;

    free(p);

    return NULL;
}
void* rul0i_scope_realloc(struct rul0i_scope* scope, void *p, size_t size)
{
    if (p == NULL) return NULL;

    void* q = realloc(p, size);

    if (scope == NULL || p == q) return p;

    if (rul0i_set_remove(scope->objs, p) && rul0i_set_add(scope->objs, q))
    {
        return q;
    }

    free(q);

    return p;
}
int rul0i_scope_free(struct rul0i_scope* scope, void* p)
{
    if (p == NULL) return 0;

    if (scope == NULL || rul0i_set_remove(scope->objs, p))
    {
        free(p);

        return 1;
    }

    return 0;
}
int rul0i_scope_add(struct rul0i_scope* scope, void* p)
{
    return scope != NULL && rul0i_set_add(scope->objs, p);
}
int rul0i_scope_has(struct rul0i_scope* scope, void* p)
{
    return scope != NULL && rul0i_set_has(scope->objs, p);
}
int rul0i_scope_remove(struct rul0i_scope* scope, void* p)
{
    return scope != NULL && rul0i_set_remove(scope->objs, p);
}
int rul0i_scope_add_str(struct rul0i_scope* scope, struct rul0i_str* p)
{
    return scope != NULL && rul0i_set_add(scope->strs, p);
}
int rul0i_scope_has_str(struct rul0i_scope* scope, struct rul0i_str* p)
{
    return scope != NULL && rul0i_set_has(scope->strs, p);
}
int rul0i_scope_remove_str(struct rul0i_scope* scope, struct rul0i_str* p)
{
    return scope != NULL && rul0i_set_remove(scope->strs, p);
}
int rul0i_scope_add_list(struct rul0i_scope* scope, struct rul0i_list* p)
{
    return scope != NULL && rul0i_set_add(scope->lists, p);
}
int rul0i_scope_has_list(struct rul0i_scope* scope, struct rul0i_list* p)
{
    return scope != NULL && rul0i_set_has(scope->lists, p);
}
int rul0i_scope_remove_list(struct rul0i_scope* scope, struct rul0i_list* p)
{
    return scope != NULL && rul0i_set_remove(scope->lists, p);
}
char* rul0i_scope_strdup(struct rul0i_scope* scope, char* s)
{
    if (s == NULL) return NULL;
    if (scope == NULL) return strdup(s);

    char *s2 = rul0i_scope_malloc(scope, strlen(s) + 1);

    if (s2 != NULL)
    {
        strcpy(s2, s);

        return s2;
    }

    rul0i_scope_free(scope, s2);

    return NULL;
}
int rul0i_state_init(struct rul0i_state *state, struct rul0i_scope *scope)
{
    if (state == NULL) return 0;

    state->code_stack = NULL;
    state->data_stack = NULL;
    state->arg0_stack = NULL;
    state->temp_stack = NULL;

    state->code_stack = rul0i_list_new(RUL0I_DATA_STACK_DEFAULT_CAP, scope);
    state->data_stack = rul0i_list_new(RUL0I_DATA_STACK_DEFAULT_CAP, scope);
    state->arg0_stack = rul0i_list_new(RUL0I_ARG0_STACK_DEFAULT_CAP, scope);
    state->temp_stack = rul0i_list_new(RUL0I_TEMP_STACK_DEFAULT_CAP, scope);

    if (state->code_stack && state->data_stack && state->arg0_stack && state->temp_stack) return 1;

    rul0i_list_unref(state->code_stack, scope);
    rul0i_list_unref(state->data_stack, scope);
    rul0i_list_unref(state->arg0_stack, scope);
    rul0i_list_unref(state->temp_stack, scope);

    state->code_stack = NULL;
    state->data_stack = NULL;
    state->arg0_stack = NULL;
    state->temp_stack = NULL;

    return 0;
}
