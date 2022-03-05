#include "rplni.h"


struct rplni_set *rplni_set_new()
{
    struct rplni_set *set = malloc(sizeof(struct rplni_set));

    if (set != NULL)
    {
        if (rplni_set_init(set)) return set;

        free(set);
    }

    return NULL;
}
int rplni_set_init(struct rplni_set *set)
{
    if (set == NULL) return 0;

    set->size = RPLNI_LIST_BLOCK_SIZE;
    size_t size2 = set->size * sizeof(void*);
    set->values = malloc(size2);

    for (size_t i = 0; i < set->size; ++i)
    {
        set->values[i] = NULL;
    }

    return set->values != NULL;
}
int rplni_set_del(struct rplni_set *set)
{
    if (set == NULL) return 0;
    if (set->values == NULL) return 1;

    free(set->values);
    free(set);

    return 1;
}
size_t rplni_set_index(struct rplni_set* set, void* value)
{
    if (set == NULL) return SIZE_MAX;

    for (size_t i = 0; i < set->size; ++i)
    {
        if (set->values[i] == value) return i;
    }

    return SIZE_MAX;
}
int rplni_set_has(struct rplni_set* set, void* value)
{
    if (set == NULL) return 0;

    return rplni_set_index(set, value) < set->size;
}
int rplni_set_add(struct rplni_set* set, void* value)
{
    if (set == NULL) return 0;
    printf("try +%p\n", value);

    if (rplni_set_has(set, value)) return 1;

    size_t idx = rplni_set_index(set, NULL);

    if (idx >= set->size)
    {
        size_t size = set->size + RPLNI_LIST_BLOCK_SIZE;
        void** a = realloc(set->values, size * sizeof(void*));

        if (a == NULL) return 0;

        for (size_t i = set->size; i < size; ++i)
        {
            a[i] = NULL;
        }

        set->values = a;
        set->size = size;
    }

    set->values[idx] = value;

    for (size_t i = 0; i < set->size; ++i)
    {
        if (set->values[i] == NULL) continue;
    }

    return 1;
}
int rplni_set_remove(struct rplni_set* set, void* value)
{
    if (set == NULL) return 0;

    printf("try -%p\n", value);

    size_t idx = rplni_set_index(set, value);

    if (idx >= set->size) return 0;

    set->values[idx] = NULL;

    if (idx + 1 < set->size)
    {
        memmove(set->values + idx, set->values + idx + 1, (set->size - idx - 1) * sizeof(void*));
    }

    return 1;
}
