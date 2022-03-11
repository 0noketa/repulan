#include "rplni.h"


struct rplni_ptrlist *rplni_ptrlist_new()
{
    struct rplni_ptrlist *list = malloc(sizeof(struct rplni_ptrlist));

    if (list != NULL)
    {
        if (rplni_ptrlist_init(list)) return list;

        free(list);
    }

    return NULL;
}
int rplni_ptrlist_init(struct rplni_ptrlist *list)
{
    if (list == NULL) return 0;

    list->size_ = RPLNI_LIST_BLOCK_SIZE;
    size_t size2 = list->size_ * sizeof(void*);
    list->values = malloc(size2);

    if (list->values == NULL)
    {
        free(list);
        return 0;
    }

    for (size_t i = 0; i < list->size_; ++i)
    {
        list->values[i] = NULL;
    }

    return list->values != NULL;
}
int rplni_ptrlist_del(struct rplni_ptrlist *list)
{
    if (list == NULL) return 0;
    if (list->values == NULL) return 1;

    free(list->values);
    free(list);

    return 1;
}
size_t rplni_ptrlist_index(struct rplni_ptrlist* list, void* value)
{
    if (list == NULL) return SIZE_MAX;

    for (size_t i = 0; i < list->size_; ++i)
    {
        if (list->values[i] == value) return i;
    }

    return list->size_;
}
int rplni_ptrlist_has(struct rplni_ptrlist* list, void* value)
{
    if (list == NULL) return 0;

    return rplni_ptrlist_index(list, value) < list->size_;
}
int rplni_ptrlist_add(struct rplni_ptrlist* list, void* value)
{
    if (list == NULL) return 0;
    printf("try +%p\n", value);

    if (rplni_ptrlist_has(list, value)) return 1;

    size_t idx = rplni_ptrlist_index(list, NULL);

    if (idx >= list->size_)
    {
        size_t size = list->size_ + RPLNI_LIST_BLOCK_SIZE;
        void** a = realloc(list->values, size * sizeof(void*));

        if (a == NULL) return 0;

        for (size_t i = list->size_; i < size; ++i)
        {
            a[i] = NULL;
        }

        list->values = a;
        list->size_ = size;
    }

    list->values[idx] = value;

    return 1;
}
int rplni_ptrlist_remove(struct rplni_ptrlist* list, void* value)
{
    if (list == NULL) return 0;

    if (list < 256) *(int*)(void*)(uintptr_t)4 = *(int*)(void*)(intptr_t)4;

    printf("try -%p @ %p\n", value, list);

    size_t idx = rplni_ptrlist_index(list, value);

    if (idx >= list->size_) return 0;

    list->values[idx] = NULL;

    if (idx + 1 < list->size_)
    {
        memmove(list->values + idx, list->values + idx + 1, (list->size_ - idx - 1) * sizeof(void*));
    }

    return 1;
}
size_t rplni_ptrlist_len(struct rplni_ptrlist* list)
{
    if (list == NULL) return 0;

    size_t idx = rplni_ptrlist_index(list, NULL);

    if (idx >= list->size_) return 0;

    return idx;
}
int rplni_ptrlist_push(struct rplni_ptrlist* list, void* value)
{
    return rplni_ptrlist_add(list, value);
}
int rplni_ptrlist_pop(struct rplni_ptrlist* list, void **out_value)
{
    if (list == NULL) return 0;

    size_t len = rplni_ptrlist_len(list);
    if (len == 0) return 0;

    printf("try -%p @ %p\n", list->values[len - 1], list);
    if (out_value != NULL)
    {
        *out_value = list->values[len - 1];
    }

    list->values[len - 1] = NULL;

    return 1;
}
int rplni_scope_export_ptrlist(
    struct rplni_scope* scope, struct rplni_ptrlist* list, struct rplni_scope* scope2,
    int ignore_elements)
{
    if (scope == NULL || list == NULL || scope2 == NULL) return 0;
    if (!rplni_scope_has(scope, list)) return 0;

    rplni_scope_add(scope2, list->values);
    rplni_scope_add(scope2, list);
    rplni_scope_remove(scope, list->values);
    rplni_scope_remove(scope, list);

    if (ignore_elements) return 1;

    for (size_t i = 0; i < list->size_; ++i)
    {
        if (list->values[i] == NULL) break;

        rplni_scope_add(scope2, list->values[i]);
        rplni_scope_remove(scope, list->values[i]);
    }

    return 1;
}
int rplni_ptrlist_clear(struct rplni_ptrlist* list)
{
    if (list == NULL) return 0;

    list->values[0] = NULL;

    return 1;
}

