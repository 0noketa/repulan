#include "rplni.h"


static struct rplni_ptrlist *rplni_ptrlist_new_(int as_strlist)
{
    struct rplni_ptrlist *list = malloc(sizeof(struct rplni_ptrlist));

    if (list != NULL)
    {
        if (rplni_ptrlist_init(list, as_strlist)) return list;

        free(list);
    }

    return NULL;
}
struct rplni_ptrlist* rplni_ptrlist_new()
{
    return rplni_ptrlist_new_(0);
}
struct rplni_ptrlist* rplni_ptrlist_new_as_strlist()
{
    return rplni_ptrlist_new_(1);
}
int rplni_ptrlist_init(struct rplni_ptrlist *list, int as_strlist)
{
    if (list == NULL) return 0;

    list->is_strlist = as_strlist;

    list->size_ = RPLNI_LIST_BLOCK_SIZE;
    size_t size2 = list->size_ * sizeof(void*);
    list->values.any = malloc(size2);

    if (list->values.any == NULL)
    {
        free(list);
        return 0;
    }

    for (size_t i = 0; i < list->size_; ++i)
    {
        list->values.any[i] = NULL;
    }

    memset(list->values.any, 0, size2);
    return list->values.any != NULL;
}
int rplni_ptrlist_del(struct rplni_ptrlist *list)
{
    if (!rplni_ptrlist_clear(list)) return 0;

    free(list->values.any);
    free(list);

    return 1;
}
size_t rplni_ptrlist_index(struct rplni_ptrlist* list, void* value)
{
    if (list == NULL) return SIZE_MAX;

    size_t i = 0;
    for (; i < list->size_ && list->values.any[i] != NULL; ++i)
    {
        if (list->values.any[i] == value) return i;
        if (list->is_strlist && value != NULL)
        {
            if (!strcmp(list->values.str[i], value)) return i;
        }
    }

    return i;
}
int rplni_ptrlist_has(struct rplni_ptrlist* list, void* value)
{
    if (list == NULL) return 0;

    size_t idx = rplni_ptrlist_index(list, value);
    return idx < list->size_ && list->values.any[idx] != NULL;
}
static int rplni_ptrlist_add_(struct rplni_ptrlist* list, void* value, int copy_str)
{
    if (list == NULL) return 0;

    if (rplni_ptrlist_has(list, value)) return 1;

    size_t idx = rplni_ptrlist_index(list, NULL);

    if (idx >= list->size_)
    {
        size_t size = list->size_ + RPLNI_LIST_BLOCK_SIZE;
        void** a = realloc(list->values.any, size * sizeof(void*));

        if (a == NULL) return 0;

        for (size_t i = list->size_; i < size; ++i)
        {
            a[i] = NULL;
        }

        list->values.any = a;
        list->size_ = size;
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

    size_t idx = rplni_ptrlist_index(list, value);

    if (idx >= list->size_ || list->values.any[idx] == NULL) return 0;

    if (list->is_strlist)
    {
        free(list->values.any[idx]);
    }
    list->values.any[idx] = NULL;

    if (idx + 1 < list->size_)
    {
        memmove(list->values.any + idx, list->values.any + idx + 1, (list->size_ - idx - 1) * sizeof(void*));
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

    size_t len = rplni_ptrlist_len(list);
    if (len == 0) return 0;

    if (out_value != NULL)
    {
        *out_value = list->values.any[len - 1];
    }
    else if (list->is_strlist)
    {
        free(list->values.cstr[len - 1]);
    }

    list->values.any[len - 1] = NULL;

    return 1;
}
int rplni_ptrlist_clear(struct rplni_ptrlist* list)
{
    if (list == NULL) return 0;

    if (!list->is_strlist)
    {
        list->values.any[0] = NULL;
        return 1;
    }

    for (size_t i = 0; i < list->size_ && list->values.any[i] != NULL; ++i)
    {
        free(list->values.any[i]);
        list->values.any[i] = NULL;
    }

    return 1;
}

