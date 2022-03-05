#include "rplni.h"


struct rplni_str *rplni_str_new(char *value, struct rplni_scope* scope)
{
    if (value == NULL) return NULL;

    struct rplni_str *str = rplni_scope_malloc(scope, sizeof(struct  rplni_str));

    if (str == NULL) return NULL;

    if (rplni_str_init(str, value, scope))
    {
        printf("+str: %p [%s]\n", str, str->value);
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
    if (str->refs == UINT32_MAX) return 0;

    str->refs++;

    return 1;
}
int rplni_str_unref(struct rplni_str* str, struct rplni_scope* scope)
{
    if (str == NULL) return 0;

    str->refs--;

    if (str->refs > 0) return 1;

    printf("-str: %p [%s]\n", str, str->value);

    rplni_scope_remove_str(scope, str);
    if (str->value != NULL) rplni_scope_free(scope, str->value);
    rplni_scope_free(scope, str);

    return 1;
}
int rplni_str_del(struct rplni_str* str, struct rplni_scope* scope)
{
    if (scope == NULL || str == NULL) return 0;
    if (!rplni_scope_has(scope, str))
    {
        str->refs--;

        return 1;
    }

    rplni_scope_remove_str(scope, str);

    if (str->value != NULL) rplni_scope_free(scope, str->value);
    rplni_scope_free(scope, str);

    return 1;
}
int rplni_scope_export_str(struct rplni_scope* scope, struct rplni_str* str, struct rplni_scope* scope2)
{
    if (str == NULL) return 0;
    if (!rplni_scope_has(scope, str)) return 1;

    rplni_scope_add(scope2, str);
    rplni_scope_add(scope2, str->value);
    rplni_scope_add_str(scope2, str);

    rplni_scope_remove(scope, str);
    rplni_scope_remove(scope, str->value);
    rplni_scope_remove_str(scope, str);

    return rplni_str_ref(str);
}
int rplni_str_add(struct rplni_str* str, struct rplni_str* str2, struct rplni_scope* scope)
{
    if (str == NULL || str2 == NULL) return 0;
    if (str2->size == 0) return 1;

    char* s = rplni_scope_realloc(scope, str->value, str->size + str2->size + 1);

    if (s == NULL) return 0;

    str->value = s;
    strcpy(str->value + str->size, str2->value);

    str->size += str2->size;

    return 1;
}
int rplni_str_add_cstr(struct rplni_str* str, char* str2, struct rplni_scope* scope)
{
    if (str == NULL || str2 == NULL) return 0;

    size_t size2 = strlen(str2);
    if (size2 == 0) return 1;

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
        printf("+list: %p\n", list);
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
        printf("+list: %p\n", list);
        rplni_scope_add_list(scope, list);
        return list;
    }

    rplni_scope_free(scope, list);

    return NULL;
}
int rplni_list_init_with_captured(struct rplni_list* list, size_t size, struct rplni_list *stack, struct rplni_scope* scope)
{
    if (list == NULL || stack == NULL || stack->size < size) return 0;

    list->refs = 0;
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
    if (list->refs == UINT32_MAX) return 0;

    list->refs++;

    return 1;
}
int rplni_list_unref(struct rplni_list* list, struct rplni_scope* scope)
{
    if (list == NULL) return 0;

    list->refs--;

    if (list->refs > 0) return 1;

    for (size_t i = 0; i < list->size; ++i)
    {
        rplni_value_clean(list->values + i, scope);
    }

    printf("-list: %p[%u]\n", list, (int)list->size);

    rplni_scope_remove_list(scope, list);
    if (list->values != NULL) rplni_scope_free(scope, list->values);
    rplni_scope_free(scope, list);

    return 1;
}
int rplni_list_del(struct rplni_list* list, struct rplni_scope* scope)
{
    if (scope == NULL || list == NULL) return 0;
    if (!rplni_scope_has(scope, list))
    {
        list->refs--;
        return 1;
    }

    rplni_scope_remove(scope, list);
    rplni_scope_remove(scope, list->values);
    rplni_scope_remove_list(scope, list);

    for (size_t i = 0; i < list->size; ++i)
    {
        rplni_value_del(scope, list->values + i);
    }

    rplni_scope_free(scope, list->values);
    rplni_scope_free(scope, list);

    return 1;
}
int rplni_scope_export_list(struct rplni_scope* scope, struct rplni_list* list, struct rplni_scope* scope2)
{
    if (list == NULL || scope == NULL || scope2 == NULL) return 0;
    if (!rplni_scope_has(scope, list)) return 1;

    rplni_scope_remove(scope, list);
    rplni_scope_remove(scope, list->values);
    rplni_scope_remove_list(scope, list);

    rplni_scope_add(scope2, list);
    rplni_scope_add(scope2, list->values);
    rplni_scope_add_list(scope2, list);

    for (size_t i = 0; i < list->size; ++i)
    {
        rplni_scope_export_value(scope, list->values + i, scope2);
    }

    return rplni_list_ref(list);
}
int rplni_list_push(struct rplni_list *list, struct rplni_value *value, struct rplni_scope* scope)
{
    if (list == NULL || value == NULL) return 0;

    if (list->size >= list->cap)
    {
        size_t new_cap = list->cap + RPLNI_LIST_BLOCK_SIZE;
        struct rplni_value *values = rplni_scope_realloc(scope, list->values, new_cap * sizeof(struct rplni_value));

        if (values == NULL) return 0;
    
        list->cap = new_cap;
        list->values = values;
    }

    if (!rplni_value_ref(value)) return 0;

    list->values[list->size++] = *value;

    return 1;
}
int rplni_list_pop(struct rplni_list *list, struct rplni_value *out_value, struct rplni_scope* scope)
{
    if (list == NULL || list->size == 0) return 0;

    if (list->cap - list->size > RPLNI_LIST_BLOCK_SIZE * 2)
    {
        size_t new_cap = list->cap - RPLNI_LIST_BLOCK_SIZE;
        struct rplni_value *values = rplni_scope_realloc(scope, list->values, new_cap * sizeof(struct rplni_value));

        if (values == NULL) return 0;

        list->cap = new_cap;
        list->values = values;
    }

    struct rplni_value value = list->values[list->size - 1];

    list->size--;
    if (out_value != NULL) *out_value = value;
    
    return 1;
}
