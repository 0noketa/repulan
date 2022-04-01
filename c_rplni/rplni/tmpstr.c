#include "rplni.h"


struct rplni_tmpstr* rplni_tmpstr_new()
{
    struct rplni_tmpstr* tmpstr = malloc(sizeof(struct rplni_tmpstr));
    if (tmpstr == NULL) return NULL;

    if (rplni_tmpstr_init(tmpstr)) return tmpstr;

    free(tmpstr);
    return NULL;
}
int rplni_tmpstr_init(struct rplni_tmpstr* tmpstr)
{
    if (tmpstr == NULL) return 0;

    char* s = malloc(RPLNI_TMPSTR_PADDING_SIZE + 1);
    if (s == NULL) return 0;

    tmpstr->cap = RPLNI_TMPSTR_PADDING_SIZE;
    tmpstr->size = 0;
    tmpstr->s = s;

    return 1;
}
int rplni_tmpstr_del(struct rplni_tmpstr* tmpstr)
{
    if (tmpstr == NULL) return 0;

    free(tmpstr->s);
    free(tmpstr);
    return 1;
}
int rplni_tmpstr_add_cstr(struct rplni_tmpstr* tmpstr, size_t size, const char* cstr)
{
    if (tmpstr == NULL || cstr == NULL) return 0;

    size_t new_size = tmpstr->size + size;
    if (tmpstr->cap < new_size)
    {
        size_t new_cap = new_size + RPLNI_TMPSTR_PADDING_SIZE;
        char* s = realloc(tmpstr->s, new_cap + 1);
        if (s == NULL) return 0;

        tmpstr->cap = new_cap;
        tmpstr->s = s;

    }

    strncpy(tmpstr->s + tmpstr->size, cstr, size);
    tmpstr->s[new_size] = 0;
    tmpstr->size = new_size;

    return 1;
}

int rplni_tmpstr_add_uintptr(struct rplni_tmpstr* tmpstr, uintptr_t u)
{
    if (tmpstr == NULL) return 0;

    size_t new_size = tmpstr->size + 48;
    if (tmpstr->cap < new_size)
    {
        size_t new_cap = new_size + RPLNI_TMPSTR_PADDING_SIZE;
        char* s = realloc(tmpstr->s, new_cap + 1);
        if (s == NULL) return 0;

        tmpstr->cap = new_cap;
        tmpstr->s = s;
    }

    tmpstr->size += sprintf(tmpstr->s + tmpstr->size, "%" PRIuPTR, u);

    return 1;

}

int rplni_tmpstr_add_ptr(struct rplni_tmpstr* tmpstr, const void* p)
{
    if (tmpstr == NULL) return 0;

    size_t new_size = tmpstr->size + 32;
    if (tmpstr->cap < new_size)
    {
        size_t new_cap = new_size + RPLNI_TMPSTR_PADDING_SIZE;
        char* s = realloc(tmpstr->s, new_cap + 1);
        if (s == NULL) return 0;

        tmpstr->cap = new_cap;
        tmpstr->s = s;
    }

    tmpstr->size += sprintf(tmpstr->s + tmpstr->size, "%p", p);

    return 1;
}


int rplni_tmpstr_add_value(struct rplni_tmpstr* tmp, const struct rplni_value* value, struct rplni_ptrlist* known_nodes)
{
    if (tmp == NULL || value == NULL) return 0;

    switch (value->type)
    {
    case RPLNI_TYPE_UINT:
        return rplni_tmpstr_add_uintptr(tmp, value->value._uint);
    case RPLNI_TYPE_STR:
        return (known_nodes == NULL)
            ? rplni_tmpstr_add_cstr(tmp, value->value._str->size, value->value._str->value)
            : rplni_tmpstr_add_str(tmp, value->value._str);
    case RPLNI_TYPE_LIST:
        return rplni_tmpstr_add_list(tmp, value->value._list, known_nodes);
    case RPLNI_TYPE_FUNC:
        return rplni_tmpstr_add_func(tmp, value->value._func);
    case RPLNI_TYPE_CLOSURE:
        return rplni_tmpstr_add_closure(tmp, value->value._closure);
    case RPLNI_TYPE_CFUNC:
    default:
        return 0;
    }
}
int rplni_tmpstr_add_str(struct rplni_tmpstr* tmp, const struct rplni_str* str)
{
    if (tmp == NULL || str == NULL) return 0;

    rplni_tmpstr_add_cstr(tmp, 1, "\"");

    for (size_t i = 0; i < str->size;)
    {
        size_t j = i + strcspn(str->value + i, "\n\r\t\"\'\\\b");
        if (i < j)
        {
            rplni_tmpstr_add_cstr(tmp, j - i, str->value + i);
        }

        if (j >= str->size) break;

        int c = str->value[j];

        rplni_tmpstr_add_cstr(tmp, 1, "\\");
        rplni_tmpstr_add_cstr(tmp,
            1,
            c == '\n' ? "n"
            : c == '\r' ? "r"
            : c == '\t' ? "t"
            : c == '\a' ? "a"
            : c == '\b' ? "b"
            : str->value + j);

        i = j + 1;
    }

    rplni_tmpstr_add_cstr(tmp, 1, "\"");

    return 1;
}

int rplni_tmpstr_add_joined_strs(struct rplni_tmpstr* tmp, struct rplni_list* list, size_t sep_size, const char* sep)
{
    if (tmp == NULL || list == NULL) return 0;

    for (size_t i = 0; i < list->size; ++i)
    {
        if (i > 0)
        {
            rplni_tmpstr_add_cstr(tmp, sep_size, sep);
        }

        rplni_tmpstr_add_value(tmp, list->values + i, NULL);
    }

    return 1;
}
int rplni_tmpstr_add_list(struct rplni_tmpstr* tmp, struct rplni_list* list, struct rplni_ptrlist* known_nodes)
{
    if (tmp == NULL || list == NULL || known_nodes == NULL) return 0;

    int is_root = known_nodes == NULL;
    if (is_root)
    {
        known_nodes = rplni_ptrlist_new(1);
        if (known_nodes == NULL) return 0;
    }
    else if (rplni_ptrlist_has(known_nodes, list))
    {
        rplni_tmpstr_add_cstr(tmp, 6, "<list:");
        rplni_tmpstr_add_ptr(tmp, list);
        rplni_tmpstr_add_cstr(tmp, 1, ">");

        return 1;
    }

    rplni_tmpstr_add_cstr(tmp, 1, "[");
    rplni_ptrlist_add(known_nodes, list);

    for (size_t i = 0; i < list->size; ++i)
    {
        if (i > 0)
        {
            rplni_tmpstr_add_cstr(tmp, 1, " ");
        }

        rplni_tmpstr_add_value(tmp, list->values + i, known_nodes);
    }

    if (is_root)
    {
        rplni_ptrlist_del(known_nodes);
    }
    else
    {
        rplni_ptrlist_remove(known_nodes, list);
    }
    rplni_tmpstr_add_cstr(tmp, 1, "]");


    return 1;
}
int rplni_tmpstr_add_func(struct rplni_tmpstr* tmp, const struct rplni_func* func)
{
    if (tmp == NULL || func == NULL) return 0;

    rplni_tmpstr_add_cstr(tmp, 6, "<func/");
    rplni_tmpstr_add_uintptr(tmp, func->params->size);
    rplni_tmpstr_add_cstr(tmp, 1, ":");
    rplni_tmpstr_add_ptr(tmp, func);
    rplni_tmpstr_add_cstr(tmp, 1, ">");

    return 1;
}
int rplni_tmpstr_add_closure(struct rplni_tmpstr* tmp, const struct rplni_closure* closure)
{
    if (tmp == NULL || closure == NULL || closure->funcdef == NULL) return 0;

    rplni_tmpstr_add_cstr(tmp, 9, "<closure/");
    rplni_tmpstr_add_uintptr(tmp, closure->funcdef->params->size);
    rplni_tmpstr_add_cstr(tmp, 1, ":");
    rplni_tmpstr_add_ptr(tmp, closure);
    rplni_tmpstr_add_cstr(tmp, 1, "*");
    rplni_tmpstr_add_ptr(tmp, closure->funcdef);
    rplni_tmpstr_add_cstr(tmp, 1, ">");

    return 1;
}


int rplni_value_to_tmpstr(const struct rplni_value* value, struct rplni_tmpstr** out_tmpstr)
{
    if (value == NULL || out_tmpstr == NULL) return 0;

    struct rplni_ptrlist* known_nodes = rplni_ptrlist_new(1);
    struct rplni_tmpstr* tmp = rplni_tmpstr_new();

    int r;
    if (value->type == RPLNI_TYPE_STR)
    {
        r = rplni_tmpstr_add_cstr(tmp, value->value._str->size, value->value._str->value);
    }
    else
    {
        r = rplni_tmpstr_add_value(tmp, value, known_nodes);
    }

    rplni_ptrlist_del(known_nodes);

    if (r)
    {
        *out_tmpstr = tmp;
        return 1;
    }

    *out_tmpstr = NULL;
    rplni_tmpstr_del(tmp);
    return 0;
}
int rplni_str_to_tmpstr(const struct rplni_str* str, struct rplni_tmpstr** out_tmpstr)
{
    if (str == NULL || out_tmpstr == NULL) return 0;

    struct rplni_tmpstr* tmp = rplni_tmpstr_new();
    if (tmp != NULL && rplni_tmpstr_add_str(tmp, str))
    {
        *out_tmpstr = tmp;
        return 1;
    }

    *out_tmpstr = NULL;
    rplni_tmpstr_del(tmp);
    return 0;
}

int rplni_list_to_tmpstr(struct rplni_list* list, struct rplni_tmpstr** out_tmpstr)
{
    if (list == NULL || out_tmpstr == NULL) return 0;

    struct rplni_ptrlist* known_nodes = rplni_ptrlist_new(1);
    struct rplni_tmpstr* tmp = rplni_tmpstr_new();

    int r = rplni_tmpstr_add_list(tmp, list, known_nodes);

    rplni_ptrlist_del(known_nodes);

    if (r)
    {
        *out_tmpstr = tmp;
        return 1;
    }

    *out_tmpstr = NULL;
    rplni_tmpstr_del(tmp);
    return 0;
}

int rplni_func_to_tmpstr(const struct rplni_func* func, struct rplni_tmpstr** out_tmpstr)
{
    if (func == NULL || out_tmpstr == NULL) return 0;

    struct rplni_tmpstr* tmp = rplni_tmpstr_new();
    if (tmp != NULL && rplni_tmpstr_add_func(tmp, func))
    {
        *out_tmpstr = tmp;
        return 1;
    }

    *out_tmpstr = NULL;
    rplni_tmpstr_del(tmp);
    return 0;
}


