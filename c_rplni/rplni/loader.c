#include <ctype.h>
#include "rplni.h"
#include "rplni_loader.h"


int rplni_loader_init(struct rplni_loader* loader)
{
    if (loader == NULL) return 0;

    loader->names = rplni_ptrlist_new_as_strlist();

    return loader->names != NULL;
}
int rplni_loader_clean(struct rplni_loader* loader)
{
    if (loader == NULL) return 0;

    rplni_ptrlist_del(loader->names);
    loader->names = NULL;

    return 1;
}

int rplni_loader_add_name(struct rplni_loader* loader, char* name, int copy_str)
{
    if (loader == NULL || name == NULL) return 0;
    if (!rplni_ptrlist_has(loader->names, name)
        && loader->names->size >= RPLNI_ID_MAX) return 0;

    if (!rplni_ptrlist_add_str(loader->names, name, copy_str)) return 0;

    return 1;
}
rplni_id_t rplni_loader_name_index(const struct rplni_loader* loader, const char* name)
{
    if (loader == NULL || name == NULL) return 0;

    size_t index = rplni_ptrlist_index(loader->names, name);

    return (rplni_id_t)index;
}
int rplni_loader_has_name(const struct rplni_loader* loader, const char* name)
{
    return rplni_ptrlist_has(loader->names, name);
}
int rplni_loader_name_by_index(struct rplni_loader* loader, rplni_id_t id, char** out_name)
{
    if (loader == NULL || id >= loader->names->size) return 0;

    if (out_name != NULL) *out_name = loader->names->values.cstr[id];

    return 1;
}

int rplni_loader_read(struct rplni_loader* loader,
    size_t size, const char *src,
    size_t* out_start, size_t *out_end, char **out_tkn, rplni_id_t* out_id)
{
    if (loader == NULL || src == NULL) return 0;

    size_t tkn_start = strspn(src, " \t\n\r");
    size_t tkn_end = tkn_start + 1;
    const char* tkn = src + tkn_start;

    if (tkn_start == tkn_end) return 0;
    if (tkn_end >= size) return 0;

    struct rplni_tmpstr *tmp = rplni_tmpstr_new(); 

    if (*tkn == '\"')
    {
        size_t i = tkn_start;
        size_t j = i + 1;
        for (; j < size && src[j] != '\"';)
        {
            if (src[j] == '\\')
            {
                rplni_tmpstr_add_cstr(tmp, j - i, src + i);
                ++j;
                i = j;
                if (j >= size) break;

                char buf[2] = {0,};
                switch (src[j])
                {
                    case 'a':  buf[0] = '\a'; break;
                    case 'b':  buf[0] = '\b'; break;
                    case 't':  buf[0] = '\t'; break;
                    case 'n':  buf[0] = '\n'; break;
                    case 'r':  buf[0] = '\r'; break;
                    default :  buf[0] = src[j]; break;
                }

                rplni_tmpstr_add_cstr(tmp, 1, buf);
                ++j;
                i = j;
            }
            else
            {
                ++j;
            }
        }

        if (i < j)
        {
            size_t str_size = j - i;

            const char* it = src + i;

            rplni_tmpstr_add_cstr(tmp, str_size, it);
        }

        if (j < size) ++j;

        const char* first = src + tkn_start;
        const char* last = src + j;

        tkn_end = j;
    }
    else
    {
        if (!strncmp(tkn, "==", 2)
            || !strncmp(tkn, "!=", 2)
            || !strncmp(tkn, "?!", 2)
            || !strncmp(tkn, "\\\\", 2)
            || !strncmp(tkn, "..", 2))
        {
            ++tkn_end;
        }
        else if (*tkn == '=' && iscsymf(tkn[1]))
        {
            while (tkn_end < size && iscsym(src[tkn_end])) ++tkn_end;
        }
        else if (iscsymf(*tkn))
        {
            while (tkn_end < size && iscsym(src[tkn_end])) ++tkn_end;
        }
        else if (isdigit(*tkn))
        {
            while (tkn_end < size && isdigit(src[tkn_end])) ++tkn_end;
        }

        size_t tkn_size = tkn_end - tkn_start;
        rplni_tmpstr_add_cstr(tmp, tkn_size, tkn);
    }

    if (rplni_loader_has_name(loader, tmp->s)
        || rplni_loader_add_name(loader, tmp->s, 1))
    {
        rplni_id_t id = rplni_loader_name_index(loader, tmp->s);
        if (out_start != NULL) *out_start = tkn_start;
        if (out_end != NULL) *out_end = tkn_end;
        if (out_tkn != NULL) rplni_loader_name_by_index(loader, id, out_tkn);
        if (out_id != NULL) *out_id = id;

        rplni_tmpstr_del(tmp);
        return 1;
    }
    else
    {
        rplni_tmpstr_del(tmp);
        return 0;
    }
}


int rplni_prog_load(
    struct rplni_prog* prog,
    size_t size, const char *src,
    const struct rplni_ptrlist* params,
    struct rplni_ptrlist* members,
    struct rplni_state* state,
    size_t *out_next)
{
    if (prog == NULL || src == NULL || state == NULL) return 0;

    size_t n_params = params == NULL ? 0 : params->size;

    struct rplni_loader loader;
    if (!rplni_loader_init(&loader)) return 0;

    for (size_t i = 0; i < size;)
    {
        size_t tkn_end;
        char* tkn;
        if (!rplni_loader_read(&loader, size - i, src + i,
                NULL, &tkn_end, &tkn, NULL))
        {
            break;
        }

        i += tkn_end;


        if (*tkn == 0) continue;

        size_t tkn_size = strlen(tkn);

        if (tkn_size == 1)
        {
            if (*tkn == '{')
            {
                struct rplni_value tmp;
                rplni_value_init_with_empty_func(&tmp, RPLNI_FUNC_BLOCK, state);

                size_t end;
                rplni_prog_load(
                    &(tmp.value._func->prog),
                    size - i, src + i,
                    NULL, NULL,
                    state,
                    &end);
                i += end;

                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_PUSH, &tmp, state);

                rplni_prog_add(prog, &cmd, state);

                rplni_value_clean(&tmp, NULL);
                continue;
            }

            if (*tkn == '}')
            {
                if (out_next != NULL) *out_next = i;
                break;
            }

            if (*tkn == '[')
            {
                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_BEGIN_LIST, NULL, state);

                rplni_prog_add(prog, &cmd, state);

                continue;
            }

            if (*tkn == ']')
            {
                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_END_LIST, NULL, state);

                rplni_prog_add(prog, &cmd, state);

                continue;
            }

            if (*tkn == '(')
            {
                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_BEGIN_ARGS, NULL, state);

                rplni_prog_add(prog, &cmd, state);

                continue;
            }

            if (*tkn == ')')
            {
                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_END_ARGS, NULL, state);

                rplni_prog_add(prog, &cmd, state);

                continue;
            }

            if (*tkn == '+')
            {
                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_ADD, NULL, state);

                rplni_prog_add(prog, &cmd, state);

                continue;
            }

            if (*tkn == '-')
            {
                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_SUB, NULL, state);

                rplni_prog_add(prog, &cmd, state);

                continue;
            }

            if (*tkn == '*')
            {
                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_MUL, NULL, state);

                rplni_prog_add(prog, &cmd, state);

                continue;
            }

            if (*tkn == '/')
            {
                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_DIV, NULL, state);

                rplni_prog_add(prog, &cmd, state);

                continue;
            }

            if (*tkn == '%')
            {
                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_MOD, NULL, state);

                rplni_prog_add(prog, &cmd, state);

                continue;
            }

            if (*tkn == '<')
            {
                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_LT, NULL, state);

                rplni_prog_add(prog, &cmd, state);

                continue;
            }

            if (*tkn == '>')
            {
                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_GT, NULL, state);

                rplni_prog_add(prog, &cmd, state);

                continue;
            }

            if (*tkn == ':')
            {
                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_RANGE, NULL, state);

                rplni_prog_add(prog, &cmd, state);

                continue;
            }


            if (*tkn == '?')
            {
                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_IF, NULL, state);

                rplni_prog_add(prog, &cmd, state);

                continue;
            }

            if (*tkn == '|')
            {
                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_ELSE, NULL, state);

                rplni_prog_add(prog, &cmd, state);

                continue;
            }

            if (*tkn == '\\')
            {
                struct rplni_value tmp;
                rplni_value_init_with_empty_func(&tmp, RPLNI_FUNC_FUNC, state);

                size_t end;
                rplni_func_load(tmp.value._func,
                    size - i, src + i,
                    state,
                    &end);
                i += end;

                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_PUSH, &tmp, state);

                rplni_prog_add(prog, &cmd, state);

                rplni_value_clean(&tmp, NULL);
                continue;
            }
        } /* end single char */


        if (!strcmp(tkn, "\\\\"))
        {
            struct rplni_value tmp;
            rplni_value_init_with_empty_func(&tmp, RPLNI_FUNC_CLOSURE, state);
            struct rplni_func* tmp_func = tmp.value._func;

            size_t end;
            rplni_func_load(tmp_func,
                size - i, src + i,
                state,
                &end);
            i += end;

            if (members != NULL)
            {
                for (size_t i = 0; i < tmp_func->members->size; ++i)
                {
                    char* name = tmp_func->members->values.cstr[i];

                    if (rplni_ptrlist_has(members, name)) continue;
                    rplni_ptrlist_add_str(members, name, 1);
                }
            }

            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_PUSH_CLOSURE, &tmp, state);
            rplni_prog_add(prog, &cmd, state);

            rplni_value_clean(&tmp, NULL);
            continue;
        }


        if (!strcmp(tkn, "__sum"))
        {
            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_SUM, NULL, state);

            rplni_prog_add(prog, &cmd, state);

            continue;
        }

        if (!strcmp(tkn, "__str"))
        {
            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_STR, NULL, state);

            rplni_prog_add(prog, &cmd, state);

            continue;
        }

        if (!strcmp(tkn, "__int"))
        {
            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_INT, NULL, state);

            rplni_prog_add(prog, &cmd, state);

            continue;
        }

        if (!strcmp(tkn, "__print"))
        {
            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_PRINT, NULL, state);

            rplni_prog_add(prog, &cmd, state);

            continue;
        }

        if (!strcmp(tkn, "__len"))
        {
            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_LEN, NULL, state);

            rplni_prog_add(prog, &cmd, state);

            continue;
        }

        if (!strcmp(tkn, "__optimize"))
        {
            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_OPTIMIZE, NULL, state);

            rplni_prog_add(prog, &cmd, state);

            continue;
        }

        if (!strcmp(tkn, ".."))
        {
            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_SPREAD, NULL, state);

            rplni_prog_add(prog, &cmd, state);

            continue;
        }

        if (!strcmp(tkn, "call"))
        {
            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_CALL, NULL, state);

            rplni_prog_add(prog, &cmd, state);

            continue;
        }

        if (!strcmp(tkn, "drop"))
        {
            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_DROP, NULL, state);

            rplni_prog_add(prog, &cmd, state);

            continue;
        }

        if (!strcmp(tkn, "dup"))
        {
            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_DUP, NULL, state);

            rplni_prog_add(prog, &cmd, state);

            continue;
        }

        if (!strcmp(tkn, "swap"))
        {
            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_SWAP, NULL, state);

            rplni_prog_add(prog, &cmd, state);

            continue;
        }

        if (!strcmp(tkn, "=="))
        {
            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_EQ, NULL, state);

            rplni_prog_add(prog, &cmd, state);

            continue;
        }

        if (!strcmp(tkn, "!="))
        {
            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_NEQ, NULL, state);

            rplni_prog_add(prog, &cmd, state);

            continue;
        }

        if (!strcmp(tkn, "?!"))
        {
            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_ENDIF, NULL, state);

            rplni_prog_add(prog, &cmd, state);

            continue;
        }

        if (!strcmp(tkn, "restart"))
        {
            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_RESTART, NULL, state);

            rplni_prog_add(prog, &cmd, state);

            continue;
        }

        if (isdigit(*tkn))
        {
            struct rplni_value tmp;
            rplni_value_init_with_uint(&tmp, (uintptr_t)atol(tkn));

            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_PUSH, &tmp, state);

            rplni_prog_add(prog, &cmd, state);

            continue;
        }

        if (*tkn == '\"')
        {
            struct rplni_value tmp;
            rplni_value_init_with_cstr(&tmp, tkn_size - 1, tkn + 1, state);

            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_PUSH, &tmp, state);

            rplni_prog_add(prog, &cmd, state);

            rplni_value_clean(&tmp, NULL);

            continue;
        }

        enum rplni_op_type var_op = RPLNI_OP_LOAD;
        if (strlen(tkn) > 1 && *tkn == '=' && iscsymf(tkn[1]))
        {
            var_op = RPLNI_OP_STORE;
            ++tkn;
        }
        if (iscsymf(*tkn))
        {
            struct rplni_value tmp;

            if (rplni_ptrlist_has(params, tkn))
            {
                size_t idx = rplni_ptrlist_index(params, tkn);
                rplni_value_init_with_uint(&tmp, (uintptr_t)idx);
            }
            else if (members != NULL)
            {
                if (!rplni_ptrlist_has(members, tkn))
                {
                    rplni_ptrlist_add_str(members, tkn, 1);
                }

                size_t idx = rplni_ptrlist_index(members, tkn);
                rplni_value_init_with_uint(&tmp, (uintptr_t)idx);
            }
            else
            {
                rplni_value_init_with_cstr(&tmp, tkn_size, tkn, state);
            }

            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, var_op, &tmp, state);

            rplni_prog_add(prog, &cmd, state);

            rplni_value_clean(&tmp, NULL);

            continue;
        }

        fprintf(stderr, "unknown token [%s](len: %d)\n", tkn, (int)strlen(tkn));
    }

    rplni_loader_clean(&loader);

    return 1;
}

int rplni_func_load(
    struct rplni_func* func,
    size_t size, const char* src,
    struct rplni_state *state,
    size_t* out_next)
{
    if (func == NULL || src == NULL || state == NULL) return 0;

    struct rplni_loader loader;
    if (!rplni_loader_init(&loader)) return 0;

    size_t i = 0;
    for (; i < size;)
    {
        size_t tkn_end;
        char* tkn;
        if (!rplni_loader_read(&loader, size - i, src + i,
            NULL, &tkn_end, &tkn, NULL))
        {
            break;
        }

        i += tkn_end;

        if (*tkn == '{' && tkn[1] == 0) break;

        rplni_func_add_param(func, tkn, 1);
        rplni_ptrlist_add_str(func->members, tkn, 1);
    }

    rplni_loader_clean(&loader);

    size_t j;
    rplni_prog_load(
        &(func->prog),
        size - i, src + i,
        func->params,
        func->type == RPLNI_FUNC_CLOSURE ? func->members : NULL,
        state,
        &j);

    if (out_next != NULL) *out_next = i + j;

    return 1;
}

int rplni_state_eval(
    struct rplni_state* state,
    size_t size, const char* src)
{
    if (state == NULL || src == NULL) return 0;

    struct rplni_value func;
    rplni_value_init_with_empty_func(&func, RPLNI_FUNC_BLOCK, state);
    rplni_prog_load(
        &func.value._func->prog,
        size, src,
        NULL, NULL,
        state,
        NULL);
    rplni_func_run(func.value._func, state, NULL);
    rplni_value_clean(&func, NULL);

    return 1;
}
