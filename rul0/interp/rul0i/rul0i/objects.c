#include "rul0i.h"


struct rul0i_str *rul0i_str_new(char *value)
{
    if (value == NULL) return NULL;

    struct rul0i_str *str = malloc(sizeof(struct  rul0i_str));

    if (str == NULL) return NULL;

    if (rul0i_str_init(str, value))
    {
        printf("+str: %p [%s]\n", str, str->value);
        return str;
    }

    free(str);

    return NULL;
}
int rul0i_str_init(struct rul0i_str *str, char *value)
{
    if (str == NULL) return 0;

    str->refs = 1;
    str->size = strlen(value);
    str->value = _strdup(value);

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
int rul0i_str_unref(struct rul0i_str *str, struct rul0i_gc_nodes *nodes)
{
    if (str == NULL) return 0;
    if (nodes != NULL && rul0i_gc_nodes_has(nodes, str)) return 1;

    str->refs--;

    if (str->refs > 0) return 1;

    if (nodes != NULL && !rul0i_gc_nodes_add(nodes, str)) return 0;

    printf("-str: %p [%s]\n", str, str->value);

    if (str->value != NULL) free(str->value);

    free(str);

    return 1;
}
int rul0i_str_add(struct rul0i_str* str, struct rul0i_str* str2)
{
    if (str == NULL || str2 == NULL) return 0;
    if (str2->size == 0) return 1;

    char* s = realloc(str->value, str->size + str2->size + 1);

    if (s == NULL) return 0;

    str->value = s;
    strcpy(str->value + str->size, str2->value);

    str->size += str2->size;

    return 1;
}
int rul0i_str_add_cstr(struct rul0i_str* str, char* str2)
{
    if (str == NULL || str2 == NULL) return 0;

    size_t size2 = strlen(str2);
    if (size2 == 0) return 1;

    char* s = realloc(str->value, str->size + size2 + 1);

    if (s == NULL) return 0;

    str->value = s;
    strcpy(str->value + str->size, str2);

    str->size += size2;

    return 1;
}

struct rul0i_list *rul0i_list_new(size_t cap)
{
    struct rul0i_list *list = malloc(sizeof(struct rul0i_list));

    if (list == NULL) return NULL;

    if (rul0i_list_init(list, cap))
    {
        printf("+list: %p\n", list);
        return list;
    }

    free(list);

    return NULL;
}
int rul0i_list_init(struct rul0i_list *list, size_t cap)
{
    if (list == NULL) return 0;

    list->refs = 0;
    list->cap = RUL0I_LIST_CEILED_SIZE(cap);
    list->size = 0;
    list->values = malloc(list->cap * sizeof(struct rul0i_value));

    if (list->values == NULL)
    {
        list->cap = 0;
        list->size = 0;

        return 0;
    }

    for (size_t i = 0; i < list->cap; ++i)
    {
        rul0i_value_init(list->values + i);
    }

    return 1;
}
int rul0i_list_ref(struct rul0i_list *list)
{
    if (list == NULL) return 0;
    if (list->refs == UINT32_MAX) return 0;

    list->refs++;

    return 1;
}
int rul0i_list_unref(struct rul0i_list *list, struct rul0i_gc_nodes *nodes)
{
    if (list == NULL) return 0;

    if (nodes != NULL && rul0i_gc_nodes_has(nodes, list)) return 1;

    list->refs--;

    if (list->refs > 0) return 1;

    int owns_nodes = nodes == NULL;

    if (owns_nodes) nodes = rul0i_gc_nodes_new();

    rul0i_gc_nodes_add(nodes, list);

    for (size_t i = 0; i < list->size; ++i)
    {
        rul0i_value_clean(list->values + i, nodes);
    }

    printf("-list: %p[%u]\n", list, (int)list->size);

    if (list->values != NULL) free(list->values);
    free(list);

    if (owns_nodes) rul0i_gc_nodes_clean(nodes);

    return 1;
}
int rul0i_list_push(struct rul0i_list *list, struct rul0i_value *value)
{
    if (list == NULL || value == NULL) return 0;

    if (list->size >= list->cap)
    {
        size_t new_cap = list->cap + RUL0I_LIST_BLOCK_SIZE;
        struct rul0i_value *values = realloc(list->values, new_cap * sizeof(struct rul0i_value));

        if (values == NULL) return 0;
    
        list->cap = new_cap;
        list->values = values;
    }

    if (!rul0i_value_ref(value)) return 0;

    list->values[list->size++] = *value;

    return 1;
}
int rul0i_list_pop(struct rul0i_list *list, struct rul0i_value *out_value)
{
    if (list == NULL || list->size == 0) return 0;

    if (list->cap - list->size > RUL0I_LIST_BLOCK_SIZE * 2)
    {
        size_t new_cap = list->cap - RUL0I_LIST_BLOCK_SIZE;
        struct rul0i_value *values = realloc(list->values, new_cap * sizeof(struct rul0i_value));

        if (values == NULL) return 0;

        list->cap = new_cap;
        list->values = values;
    }

    struct rul0i_value value = list->values[list->size - 1];

    list->size--;
    if (out_value != NULL) *out_value = value;
    
    return 1;
}
