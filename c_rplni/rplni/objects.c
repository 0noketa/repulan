#include "rul0i.h"


struct rul0i_str *rul0i_str_new(char *value, struct rul0i_scope* scope)
{
    if (value == NULL) return NULL;

    struct rul0i_str *str = rul0i_scope_malloc(scope, sizeof(struct  rul0i_str));

    if (str == NULL) return NULL;

    if (rul0i_str_init(str, value, scope))
    {
        printf("+str: %p [%s]\n", str, str->value);
        rul0i_scope_add_str(scope, str);

        return str;
    }

    rul0i_scope_free(scope, str);

    return NULL;
}
int rul0i_str_init(struct rul0i_str *str, char *value, struct rul0i_scope* scope)
{
    if (str == NULL) return 0;

    str->refs = 1;
    str->size = strlen(value);
    str->value = rul0i_scope_strdup(scope, value);

    if (str->value == NULL)
    {
        str->size = 0;

        return 0;
    }

    return 1;
}
int rul0i_str_ref(struct rul0i_str *str)
{
    if (str == NULL) return 0;
    if (str->refs == UINT32_MAX) return 0;

    str->refs++;

    return 1;
}
int rul0i_str_unref(struct rul0i_str* str, struct rul0i_scope* scope)
{
    if (str == NULL) return 0;

    str->refs--;

    if (str->refs > 0) return 1;

    printf("-str: %p [%s]\n", str, str->value);

    rul0i_scope_remove_str(scope, str);
    if (str->value != NULL) rul0i_scope_free(scope, str->value);
    rul0i_scope_free(scope, str);

    return 1;
}
int rul0i_str_del(struct rul0i_str* str, struct rul0i_scope* scope)
{
    if (scope == NULL || str == NULL) return 0;
    if (!rul0i_scope_has(scope, str))
    {
        str->refs--;

        return 1;
    }

    rul0i_scope_remove_str(scope, str);

    if (str->value != NULL) rul0i_scope_free(scope, str->value);
    rul0i_scope_free(scope, str);

    return 1;
}
int rul0i_scope_export_str(struct rul0i_scope* scope, struct rul0i_str* str, struct rul0i_scope* scope2)
{
    if (str == NULL) return 0;
    if (!rul0i_scope_has(scope, str)) return 1;

    rul0i_scope_add(scope2, str);
    rul0i_scope_add(scope2, str->value);
    rul0i_scope_add_str(scope2, str);

    rul0i_scope_remove(scope, str);
    rul0i_scope_remove(scope, str->value);
    rul0i_scope_remove_str(scope, str);

    return rul0i_str_ref(str);
}
int rul0i_str_add(struct rul0i_str* str, struct rul0i_str* str2, struct rul0i_scope* scope)
{
    if (str == NULL || str2 == NULL) return 0;
    if (str2->size == 0) return 1;

    char* s = rul0i_scope_realloc(scope, str->value, str->size + str2->size + 1);

    if (s == NULL) return 0;

    str->value = s;
    strcpy(str->value + str->size, str2->value);

    str->size += str2->size;

    return 1;
}
int rul0i_str_add_cstr(struct rul0i_str* str, char* str2, struct rul0i_scope* scope)
{
    if (str == NULL || str2 == NULL) return 0;

    size_t size2 = strlen(str2);
    if (size2 == 0) return 1;

    char* s = rul0i_scope_realloc(scope, str->value, str->size + size2 + 1);

    if (s == NULL) return 0;

    str->value = s;
    strcpy(str->value + str->size, str2);

    str->size += size2;

    return 1;
}

struct rul0i_list *rul0i_list_new(size_t cap, struct rul0i_scope* scope)
{
    struct rul0i_list *list = rul0i_scope_malloc(scope, sizeof(struct rul0i_list));

    if (list == NULL) return NULL;

    if (rul0i_list_init(list, cap, scope))
    {
        printf("+list: %p\n", list);
        rul0i_scope_add_list(scope, list);
        return list;
    }

    rul0i_scope_free(scope, list);

    return NULL;
}
int rul0i_list_init(struct rul0i_list *list, size_t cap, struct rul0i_scope* scope)
{
    if (list == NULL) return 0;

    list->refs = 1;
    list->cap = RUL0I_LIST_CEILED_SIZE(cap);
    list->size = 0;
    list->values = rul0i_scope_malloc(scope, list->cap * sizeof(struct rul0i_value));

    if (list->values == NULL)
    {
        list->cap = 0;
        list->size = 0;

        return 0;
    }

    rul0i_values_init(list->cap, list->values);

    return 1;
}
struct rul0i_list* rul0i_list_new_with_captured(size_t size, struct rul0i_list *stack, struct rul0i_scope* scope)
{
    struct rul0i_list* list = rul0i_scope_malloc(scope, sizeof(struct rul0i_list));

    if (list == NULL) return NULL;

    if (rul0i_list_init_with_captured(list, size, stack, scope))
    {
        printf("+list: %p\n", list);
        rul0i_scope_add_list(scope, list);
        return list;
    }

    rul0i_scope_free(scope, list);

    return NULL;
}
int rul0i_list_init_with_captured(struct rul0i_list* list, size_t size, struct rul0i_list *stack, struct rul0i_scope* scope)
{
    if (list == NULL || stack == NULL || stack->size < size) return 0;

    list->refs = 0;
    list->cap = RUL0I_LIST_CEILED_SIZE(size);
    list->size = size;
    list->values = rul0i_scope_malloc(scope, list->cap * sizeof(struct rul0i_value));

    if (list->values == NULL)
    {
        list->cap = 0;
        list->size = 0;

        return 0;
    }

    memcpy(list->values, stack->values + stack->size - size, size * sizeof(struct rul0i_value));
    stack->size -= size;

    return 1;
}
int rul0i_list_ref(struct rul0i_list *list)
{
    if (list == NULL) return 0;
    if (list->refs == UINT32_MAX) return 0;

    list->refs++;

    return 1;
}
int rul0i_list_unref(struct rul0i_list* list, struct rul0i_scope* scope)
{
    if (list == NULL) return 0;

    list->refs--;

    if (list->refs > 0) return 1;

    for (size_t i = 0; i < list->size; ++i)
    {
        rul0i_value_clean(list->values + i, scope);
    }

    printf("-list: %p[%u]\n", list, (int)list->size);

    rul0i_scope_remove_list(scope, list);
    if (list->values != NULL) rul0i_scope_free(scope, list->values);
    rul0i_scope_free(scope, list);

    return 1;
}
int rul0i_list_del(struct rul0i_list* list, struct rul0i_scope* scope)
{
    if (scope == NULL || list == NULL) return 0;
    if (!rul0i_scope_has(scope, list))
    {
        list->refs--;
        return 1;
    }

    rul0i_scope_remove(scope, list);
    rul0i_scope_remove(scope, list->values);
    rul0i_scope_remove_list(scope, list);

    for (size_t i = 0; i < list->size; ++i)
    {
        rul0i_value_del(scope, list->values + i);
    }

    rul0i_scope_free(scope, list->values);
    rul0i_scope_free(scope, list);

    return 1;
}
int rul0i_scope_export_list(struct rul0i_scope* scope, struct rul0i_list* list, struct rul0i_scope* scope2)
{
    if (list == NULL || scope == NULL || scope2 == NULL) return 0;
    if (!rul0i_scope_has(scope, list)) return 1;

    rul0i_scope_remove(scope, list);
    rul0i_scope_remove(scope, list->values);
    rul0i_scope_remove_list(scope, list);

    rul0i_scope_add(scope2, list);
    rul0i_scope_add(scope2, list->values);
    rul0i_scope_add_list(scope2, list);

    for (size_t i = 0; i < list->size; ++i)
    {
        rul0i_scope_export_value(scope, list->values + i, scope2);
    }

    return rul0i_list_ref(list);
}
int rul0i_list_push(struct rul0i_list *list, struct rul0i_value *value, struct rul0i_scope* scope)
{
    if (list == NULL || value == NULL) return 0;

    if (list->size >= list->cap)
    {
        size_t new_cap = list->cap + RUL0I_LIST_BLOCK_SIZE;
        struct rul0i_value *values = rul0i_scope_realloc(scope, list->values, new_cap * sizeof(struct rul0i_value));

        if (values == NULL) return 0;
    
        list->cap = new_cap;
        list->values = values;
    }

    if (!rul0i_value_ref(value)) return 0;

    list->values[list->size++] = *value;

    return 1;
}
int rul0i_list_pop(struct rul0i_list *list, struct rul0i_value *out_value, struct rul0i_scope* scope)
{
    if (list == NULL || list->size == 0) return 0;

    if (list->cap - list->size > RUL0I_LIST_BLOCK_SIZE * 2)
    {
        size_t new_cap = list->cap - RUL0I_LIST_BLOCK_SIZE;
        struct rul0i_value *values = rul0i_scope_realloc(scope, list->values, new_cap * sizeof(struct rul0i_value));

        if (values == NULL) return 0;

        list->cap = new_cap;
        list->values = values;
    }

    struct rul0i_value value = list->values[list->size - 1];

    list->size--;
    if (out_value != NULL) *out_value = value;
    
    return 1;
}
