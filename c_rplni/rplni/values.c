#include "rplni.h"


int rplni_value_init(struct rplni_value *value)
{
    if (value == NULL) return 0;

    value->type = RPLNI_UINT;
    value->value._uint = 0;

    return 1;
}
int rplni_value_clean(struct rplni_value *value, struct rplni_scope *scope)
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
        : value->value._uint == value2->value._uint;
}
int rplni_value_del(struct rplni_value* value, struct rplni_scope* scope)
{
    if (scope == NULL || value == NULL) return 0;

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
    default:
        return 0;
    }

    value->type = RPLNI_UINT;
    value->value._uint = 0;

    return 1;
}
int rplni_scope_export_value(struct rplni_scope* scope, struct rplni_value* value, struct rplni_scope* scope2)
{
    if (scope == NULL || value == NULL) return 0;

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
    default:
        return 0;
    }

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
        struct rplni_value* values2 = rplni_scope_realloc(scope, values, size2);

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

    named->name = rplni_scope_malloc(scope, strlen(name) + 1);

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

    rplni_scope_free(scope, named->name);
    named->name = NULL;

    return 1;
}

int rplni_scope_init(struct rplni_scope *scope)
{
    if (scope == NULL) return 0;

    scope->size = 0;
    scope->objs = rplni_set_new();
    scope->strs = rplni_set_new();
    scope->lists = rplni_set_new();
    scope->vars = rplni_scope_malloc(scope, 0);

    return scope->vars != NULL;
}
int rplni_scope_clean(struct rplni_scope *scope)
{
    if (scope == NULL) return 0;

    for (size_t i = 0; i < scope->size; ++i)
    {
        rplni_named_clean(scope->vars + i, scope);
    }
    rplni_scope_free(scope, scope->vars);
    scope->size = 0;
    scope->vars = NULL;

    printf("clean scope %p\n", scope);
    
    for (size_t i = 0; i < scope->lists->size; ++i)
    {
        struct rplni_list* obj = scope->lists->values[i];
        if (obj == NULL) continue;

        printf("-list%p\n", obj);
        rplni_list_del(obj, scope);
    }
    rplni_set_del(scope->lists);
    scope->lists = NULL;

    for (size_t i = 0; i < scope->strs->size; ++i)
    {
        struct rplni_list* obj = scope->strs->values[i];
        if (obj == NULL) continue;

        printf("-str%p\n", obj);
        rplni_str_del(obj, scope);
    }
    rplni_set_del(scope->strs);
    scope->strs = NULL;

    for (size_t i = 0; i < scope->objs->size; ++i)
    {
        void *p = scope->objs->values[i];
        if (p == NULL) continue;

        printf("-%p\n", p);
        rplni_scope_free(scope, p);
    }
    rplni_set_del(scope->objs);
    scope->objs = NULL;

    return 1;
}
void *rplni_scope_malloc(struct rplni_scope* scope, size_t size)
{
    void *p = malloc(size);

    if (scope == NULL) return p;

    if (rplni_set_add(scope->objs, p)) return p;

    free(p);

    return NULL;
}
void* rplni_scope_realloc(struct rplni_scope* scope, void *p, size_t size)
{
    if (p == NULL) return NULL;

    void* q = realloc(p, size);

    if (scope == NULL || p == q) return p;

    if (rplni_set_remove(scope->objs, p) && rplni_set_add(scope->objs, q))
    {
        return q;
    }

    free(q);

    return p;
}
int rplni_scope_free(struct rplni_scope* scope, void* p)
{
    if (p == NULL) return 0;

    if (scope == NULL || rplni_set_remove(scope->objs, p))
    {
        free(p);

        return 1;
    }

    return 0;
}
int rplni_scope_add(struct rplni_scope* scope, void* p)
{
    return scope != NULL && rplni_set_add(scope->objs, p);
}
int rplni_scope_has(struct rplni_scope* scope, void* p)
{
    return scope != NULL && rplni_set_has(scope->objs, p);
}
int rplni_scope_remove(struct rplni_scope* scope, void* p)
{
    return scope != NULL && rplni_set_remove(scope->objs, p);
}
int rplni_scope_add_str(struct rplni_scope* scope, struct rplni_str* p)
{
    return scope != NULL && rplni_set_add(scope->strs, p);
}
int rplni_scope_has_str(struct rplni_scope* scope, struct rplni_str* p)
{
    return scope != NULL && rplni_set_has(scope->strs, p);
}
int rplni_scope_remove_str(struct rplni_scope* scope, struct rplni_str* p)
{
    return scope != NULL && rplni_set_remove(scope->strs, p);
}
int rplni_scope_add_list(struct rplni_scope* scope, struct rplni_list* p)
{
    return scope != NULL && rplni_set_add(scope->lists, p);
}
int rplni_scope_has_list(struct rplni_scope* scope, struct rplni_list* p)
{
    return scope != NULL && rplni_set_has(scope->lists, p);
}
int rplni_scope_remove_list(struct rplni_scope* scope, struct rplni_list* p)
{
    return scope != NULL && rplni_set_remove(scope->lists, p);
}
char* rplni_scope_strdup(struct rplni_scope* scope, char* s)
{
    if (s == NULL) return NULL;
    if (scope == NULL) return strdup(s);

    char *s2 = rplni_scope_malloc(scope, strlen(s) + 1);

    if (s2 != NULL)
    {
        strcpy(s2, s);

        return s2;
    }

    rplni_scope_free(scope, s2);

    return NULL;
}
int rplni_state_init(struct rplni_state *state, struct rplni_scope *scope)
{
    if (state == NULL) return 0;

    state->code_stack = NULL;
    state->data_stack = NULL;
    state->arg0_stack = NULL;
    state->temp_stack = NULL;

    state->code_stack = rplni_list_new(RPLNI_DATA_STACK_DEFAULT_CAP, scope);
    state->data_stack = rplni_list_new(RPLNI_DATA_STACK_DEFAULT_CAP, scope);
    state->arg0_stack = rplni_list_new(RPLNI_ARG0_STACK_DEFAULT_CAP, scope);
    state->temp_stack = rplni_list_new(RPLNI_TEMP_STACK_DEFAULT_CAP, scope);

    if (state->code_stack && state->data_stack && state->arg0_stack && state->temp_stack) return 1;

    rplni_list_unref(state->code_stack, scope);
    rplni_list_unref(state->data_stack, scope);
    rplni_list_unref(state->arg0_stack, scope);
    rplni_list_unref(state->temp_stack, scope);

    state->code_stack = NULL;
    state->data_stack = NULL;
    state->arg0_stack = NULL;
    state->temp_stack = NULL;

    return 0;
}
