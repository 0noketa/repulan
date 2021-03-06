#include "rplni.h"


#ifndef NDEBUG
extern struct rplni_ptrlist* all_objs;
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(...)
#endif


static struct rplni_ptrlist *rplni_ptrlist_new_(int as_strlist, int as_uniquelist, struct rplni_state* state)
{
    struct rplni_ptrlist *list = rplni_state_malloc(state, sizeof(struct rplni_ptrlist));

    if (list != NULL)
    {
        if (rplni_ptrlist_init(list, as_strlist, as_uniquelist, state)) return list;

        rplni_state_free(state, list);
    }

    return NULL;
}
struct rplni_ptrlist* rplni_ptrlist_new(int as_uniquelist, struct rplni_state* state)
{
    return rplni_ptrlist_new_(0, as_uniquelist, state);
}
struct rplni_ptrlist* rplni_ptrlist_new_as_strlist(struct rplni_state* state)
{
    return rplni_ptrlist_new_(1, 1, state);
}
int rplni_ptrlist_init(struct rplni_ptrlist *list, int as_strlist, int as_uniquelist, struct rplni_state* state)
{
    if (list == NULL) return 0;

    list->is_strlist = as_strlist;
    list->is_uniquelist = as_uniquelist;

    list->cap = RPLNI_LIST_BLOCK_SIZE;
    list->size = 0;
    list->values.any = rplni_state_malloc(state, list->cap * sizeof(void*));
    
    return list->values.any != NULL;
}
int rplni_ptrlist_del(struct rplni_ptrlist *list, struct rplni_state* state)
{
    if (!rplni_ptrlist_clear(list, state)) return 0;

    rplni_state_free(state, list->values.any);
    rplni_state_free(state, list);

    return 1;
}
size_t rplni_ptrlist_index(const struct rplni_ptrlist* list, const void* value)
{
    if (list == NULL) return SIZE_MAX;

    size_t i = 0;
    if (list->is_strlist)
    {
        for (; i < list->size && list->values.any[i] != NULL; ++i)
        {
            if (list->values.any[i] == value) return i;
            if (value != NULL && !strcmp(list->values.cstr[i], value)) return i;
        }
    }
    else
    {
        for (; i < list->size && list->values.any[i] != value; ++i);
    }
    return i;
}
size_t rplni_ptrlist_uint_index(const struct rplni_ptrlist* list, uintptr_t value)
{
    return rplni_ptrlist_index(list, (const void*)value);
}
int rplni_ptrlist_has_uint(const struct rplni_ptrlist* list, uintptr_t value)
{
    return rplni_ptrlist_has(list, (const void*)value);
}
int rplni_ptrlist_has(const struct rplni_ptrlist* list, const void* value)
{
    if (list == NULL) return 0;

    size_t idx = rplni_ptrlist_index(list, value);
    return idx < list->size && list->values.any[idx] != NULL;
}
static int rplni_ptrlist_add_(struct rplni_ptrlist* list, void* value, int copy_str, struct rplni_state* state)
{
    if (list == NULL) return 0;

    if (list->is_uniquelist && rplni_ptrlist_has(list, value)) return 1;

    if (list->size >= list->cap)
    {
        size_t new_cap = list->cap + RPLNI_LIST_BLOCK_SIZE;
        void** a = rplni_state_realloc(state, list->values.any, new_cap * sizeof(void*));

        if (a == NULL) return 0;

        list->values.any = a;
        list->cap = new_cap;
    }

    if (copy_str)
    {
        list->values.cstr[list->size] = rplni_state_strdup(state, strlen(value), value);
    }
    else
    {
        list->values.any[list->size] = value;
    }

    list->size++;

    return 1;
}
int rplni_ptrlist_add(struct rplni_ptrlist* list, void* value, struct rplni_state* state)
{
    return rplni_ptrlist_add_(list, value, 0, state);
}
int rplni_ptrlist_add_uint(struct rplni_ptrlist* list, uintptr_t value, struct rplni_state* state)
{
    return rplni_ptrlist_add_(list, (void*)value, 0, state);
}
int rplni_ptrlist_add_str(struct rplni_ptrlist* list, char* value, int copy_str, struct rplni_state* state)
{
    return rplni_ptrlist_add_(list, value, copy_str, state);
}
int rplni_ptrlist_remove(struct rplni_ptrlist* list, void* value, struct rplni_state* state)
{
    if (list == NULL || value == NULL) return 0;

    size_t idx = rplni_ptrlist_index(list, value);
    if (idx >= list->size) return 0;

    if (list->is_strlist)
    {
        rplni_state_free(state, list->values.any[idx]);
    }

    if (idx + 1 < list->size)
    {
        memmove(list->values.any + idx, list->values.any + idx + 1, (list->size - idx - 1) * sizeof(void*));
    }

    list->size--;

    return 1;
}
static int rplni_ptrlist_push_(struct rplni_ptrlist* list, void* value, int copy_str, struct rplni_state* state)
{
    return rplni_ptrlist_add_(list, value, copy_str, state);
}
int rplni_ptrlist_push(struct rplni_ptrlist* list, void* value, struct rplni_state* state)
{
    return rplni_ptrlist_push_(list, value, 0, state);
}
int rplni_ptrlist_push_uint(struct rplni_ptrlist* list, uintptr_t value, struct rplni_state* state)
{
    return rplni_ptrlist_push_(list, (void*)value, 0, state);
}
int rplni_ptrlist_push_str(struct rplni_ptrlist* list, char* value, int copy_str, struct rplni_state* state)
{
    return rplni_ptrlist_push_(list, value, copy_str, state);
}
int rplni_ptrlist_pop(struct rplni_ptrlist* list, void **out_value, struct rplni_state* state)
{
    if (list == NULL) return 0;

    size_t len = list->size;
    if (len == 0) return 0;

    if (out_value != NULL)
    {
        *out_value = list->values.any[len - 1];
    }
    else if (list->is_strlist)
    {
        rplni_state_free(state, list->values.cstr[len - 1]);
    }

    list->size--;

    return 1;
}
int rplni_ptrlist_clear(struct rplni_ptrlist* list, struct rplni_state* state)
{
    if (list == NULL) return 0;

    if (!list->is_strlist)
    {
        list->size = 0;
        return 1;
    }

    for (size_t i = 0; i < list->size; ++i)
    {
        rplni_state_free(state, list->values.any[i]);
    }

    list->size = 0;
    return 1;
}
