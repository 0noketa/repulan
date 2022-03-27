#include "rplni.h"


#ifndef NDEBUG
extern struct rplni_ptrlist* all_objs;
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(...)
#endif


static struct rplni_ptrlist *rplni_ptrlist_new_(int as_strlist, int as_uniquelist)
{
    struct rplni_ptrlist *list = malloc(sizeof(struct rplni_ptrlist));

    if (list != NULL)
    {
        if (rplni_ptrlist_init(list, as_strlist, as_uniquelist)) return list;

        free(list);
    }

    return NULL;
}
struct rplni_ptrlist* rplni_ptrlist_new(int as_uniquelist)
{
    return rplni_ptrlist_new_(0, as_uniquelist);
}
struct rplni_ptrlist* rplni_ptrlist_new_as_strlist()
{
    return rplni_ptrlist_new_(1, 1);
}
int rplni_ptrlist_init(struct rplni_ptrlist *list, int as_strlist, int as_uniquelist)
{
    if (list == NULL) return 0;

    list->is_strlist = as_strlist;
    list->is_uniquelist = as_uniquelist;

    list->cap = RPLNI_LIST_BLOCK_SIZE;
    list->size = 0;
    list->values.any = malloc(list->cap * sizeof(void*));
    
    return list->values.any != NULL;
}
int rplni_ptrlist_del(struct rplni_ptrlist *list)
{
    if (!rplni_ptrlist_clear(list)) return 0;

    free(list->values.any);
    free(list);

    return 1;
}
size_t rplni_ptrlist_index(const struct rplni_ptrlist* list, const void* value)
{
    if (list == NULL) return SIZE_MAX;

    size_t i = 0;
    for (; i < list->size && list->values.any[i] != NULL; ++i)
    {
        if (list->values.any[i] == value) return i;
        if (list->is_strlist && value != NULL)
        {
            if (!strcmp(list->values.cstr[i], value)) return i;
        }
    }

    return i;
}
int rplni_ptrlist_has(const struct rplni_ptrlist* list, const void* value)
{
    if (list == NULL) return 0;

    size_t idx = rplni_ptrlist_index(list, value);
    return idx < list->size && list->values.any[idx] != NULL;
}
static int rplni_ptrlist_add_(struct rplni_ptrlist* list, void* value, int copy_str)
{
    if (list == NULL) return 0;

    if (list->is_uniquelist && rplni_ptrlist_has(list, value)) return 1;

    size_t idx = rplni_ptrlist_index(list, NULL);
    if (idx >= list->cap)
    {
        size_t new_cap = list->cap + RPLNI_LIST_BLOCK_SIZE;
        void** a = realloc(list->values.any, new_cap * sizeof(void*));

        if (a == NULL) return 0;

        list->values.any = a;
        list->cap = new_cap;
    }

    if (copy_str)
    {
        char* value2 = malloc(strlen(value) + 1);
        if (value2 == NULL) return 0;

        strcpy(value2, value);
        list->values.cstr[idx] = value2;
    }
    else
    {
        list->values.any[idx] = value;
    }

    list->size++;

    return 1;
}
int rplni_ptrlist_add(struct rplni_ptrlist* list, void* value)
{
    return rplni_ptrlist_add_(list, value, 0);
}
int rplni_ptrlist_add_str(struct rplni_ptrlist* list, char* value, int copy_str)
{
    return rplni_ptrlist_add_(list, value, copy_str);
}
int rplni_ptrlist_remove(struct rplni_ptrlist* list, void* value)
{
    if (list == NULL) return 0;
    if (list->is_strlist && value == NULL) return 0;

    size_t idx = rplni_ptrlist_index(list, value);
    if (idx >= list->size) return 0;

    if (list->is_strlist)
    {
        free(list->values.any[idx]);
    }
    list->values.any[idx] = NULL;

    if (idx + 1 < list->size)
    {
        memmove(list->values.any + idx, list->values.any + idx + 1, (list->size - idx - 1) * sizeof(void*));
    }

    list->size--;

    return 1;
}
static int rplni_ptrlist_push_(struct rplni_ptrlist* list, void* value, int copy_str)
{
    return rplni_ptrlist_add_(list, value, copy_str);
}
int rplni_ptrlist_push(struct rplni_ptrlist* list, void* value)
{
    return rplni_ptrlist_push_(list, value, 0);
}
int rplni_ptrlist_push_str(struct rplni_ptrlist* list, char* value, int copy_str)
{
    return rplni_ptrlist_push_(list, value, copy_str);
}
int rplni_ptrlist_pop(struct rplni_ptrlist* list, void **out_value)
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
        free(list->values.cstr[len - 1]);
    }

    list->size--;

    return 1;
}
int rplni_ptrlist_clear(struct rplni_ptrlist* list)
{
    if (list == NULL) return 0;

    if (!list->is_strlist)
    {
        list->size = 0;
        return 1;
    }

    for (size_t i = 0; i < list->size; ++i)
    {
        free(list->values.any[i]);
    }

    list->size = 0;
    return 1;
}
