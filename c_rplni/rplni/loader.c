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
        && rplni_ptrlist_len(loader->names) >= RPLNI_ID_MAX) return 0;

    if (!rplni_ptrlist_add_str(loader->names, name, copy_str)) return 0;

    return 1;
}
rplni_id_t rplni_loader_name_index(struct rplni_loader* loader, char* name)
{
    if (loader == NULL || name == NULL) return 0;

    size_t index = rplni_ptrlist_index(loader->names, name);

    return (rplni_id_t)index;
}
int rplni_loader_has_name(struct rplni_loader* loader, char* name)
{
    return rplni_ptrlist_has(loader->names, name);
}
int rplni_loader_name_by_index(struct rplni_loader* loader, rplni_id_t id, char** out_name)
{
    if (loader == NULL || id >= loader->names->size_) return 0;

    if (out_name != NULL) *out_name = loader->names->values.cstr[id];

    return 1;
}

int rplni_loader_read(struct rplni_loader* loader,
    size_t size, char *src,
    size_t* out_start, size_t *out_end, char **out_tkn, rplni_id_t* out_id)
{
    if (loader == NULL || src == NULL) return 0;

    size_t tkn_start = strspn(src, " \t\n\r");
    size_t tkn_end = tkn_start + 1;
    const char* tkn = src + tkn_start;

    if (tkn_start == tkn_end) return 0;

    if (tkn_end < size)
    {
        if (!strncmp(tkn, "==", 2)
            || !strncmp(tkn, "!=", 2)
            || !strncmp(tkn, "?!", 2)
            || !strncmp(tkn, "\\\\", 2))
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
        else if (*tkn == '\"')
        {
            /* no esc */
            tkn_end = tkn_start + strcspn(tkn + 1, "\"") + 2;
        }
    }

    size_t tkn_size = tkn_end - tkn_start;
    if (*tkn == '\"') --tkn_size;

    char* buf = malloc(tkn_size + 1);
    if (buf == NULL) return 0;

    strncpy(buf, tkn, tkn_size);
    buf[tkn_size] = 0;

    if (rplni_loader_has_name(loader, buf))
    {
        rplni_id_t id = rplni_loader_name_index(loader, buf);
        if (out_start != NULL) *out_start = tkn_start;
        if (out_end != NULL) *out_end = tkn_end;
        if (out_tkn != NULL) rplni_loader_name_by_index(loader, id, out_tkn);
        if (out_id != NULL) *out_id = id;

        free(buf);
        return 1;
    }
    else if (!rplni_loader_add_name(loader, buf, 0))
    {
        free(buf);
        return 0;
    }

    if (out_start != NULL) *out_start = tkn_start;
    if (out_end != NULL) *out_end = tkn_end;
    if (out_tkn != NULL) *out_tkn = buf;
    if (out_id != NULL) *out_id = rplni_loader_name_index(loader, buf);

    return 1;
}


int rplni_prog_load(
    struct rplni_prog* prog,
    size_t size, char *src,
    struct rplni_ptrlist *params, struct rplni_scope* scope,
    size_t *out_next)
{
    if (prog == NULL || src == NULL || scope == NULL) return 0;

    struct rplni_loader loader;
    if (!rplni_loader_init(&loader)) return 0;

    for (size_t i = 0; i < size;)
    {
        size_t tkn_start;
        size_t tkn_end;
        char* tkn;
        if (!rplni_loader_read(&loader, size - i, src + i,
                NULL, &tkn_end, &tkn, NULL))
        {
            break;
        }

        i += tkn_end;


        if (strlen(tkn) == 1)
        {
            if (*tkn == '{')
            {
                struct rplni_value tmp;
                rplni_value_init_with_empty_func(&tmp, RPLNI_FUNC_BLOCK, scope);

                size_t end;
                rplni_prog_load(&(tmp.value._func->prog),
                    size - i, src + i,
                    NULL,
                    scope,
                    &end);
                i += end;

                char* src2 = src + i;

                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_PUSH, &tmp, scope);

                rplni_prog_add(prog, &cmd, scope);

                rplni_value_clean(&tmp, scope);
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
                rplni_cmd_init(&cmd, RPLNI_OP_BEGIN_LIST, NULL, scope);

                rplni_prog_add(prog, &cmd, scope);

                continue;
            }

            if (*tkn == ']')
            {
                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_END_LIST, NULL, scope);

                rplni_prog_add(prog, &cmd, scope);

                continue;
            }

            if (*tkn == '(')
            {
                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_BEGIN_ARGS, NULL, scope);

                rplni_prog_add(prog, &cmd, scope);

                continue;
            }

            if (*tkn == ')')
            {
                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_END_ARGS, NULL, scope);

                rplni_prog_add(prog, &cmd, scope);

                continue;
            }

            if (*tkn == '+')
            {
                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_ADD, NULL, scope);

                rplni_prog_add(prog, &cmd, scope);

                continue;
            }

            if (*tkn == '-')
            {
                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_SUB, NULL, scope);

                rplni_prog_add(prog, &cmd, scope);

                continue;
            }

            if (*tkn == '*')
            {
                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_MUL, NULL, scope);

                rplni_prog_add(prog, &cmd, scope);

                continue;
            }

            if (*tkn == ':')
            {
                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_RANGE, NULL, scope);

                rplni_prog_add(prog, &cmd, scope);

                continue;
            }


            if (*tkn == '?')
            {
                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_IF, NULL, scope);

                rplni_prog_add(prog, &cmd, scope);

                continue;
            }

            if (*tkn == '|')
            {
                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_ELSE, NULL, scope);

                rplni_prog_add(prog, &cmd, scope);

                continue;
            }

            if (*tkn == '\\')
            {
                struct rplni_value tmp;
                rplni_value_init_with_empty_func(&tmp, RPLNI_FUNC_FUNC, scope);

                char* body = src + i;
                size_t end;
                rplni_func_load(tmp.value._func,
                    size - i, src + i,
                    NULL,
                    scope,
                    &end);
                i += end;

                for (size_t i = 0; i < tmp.value._func->prog.size; ++i)
                {
                    struct rplni_cmd* cmd = tmp.value._func->prog.code + i;
                    printf("");
                }

                char* src2 = src + i;

                struct rplni_cmd cmd;
                rplni_cmd_init(&cmd, RPLNI_OP_PUSH, &tmp, scope);

                rplni_prog_add(prog, &cmd, scope);

                rplni_value_clean(&tmp, scope);
                continue;
            }
        } /* end single char */


        if (!strcmp(tkn, "__print"))
        {
            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_PRINT, NULL, scope);

            rplni_prog_add(prog, &cmd, scope);

            continue;
        }

        if (!strcmp(tkn, "__len"))
        {
            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_LEN, NULL, scope);

            rplni_prog_add(prog, &cmd, scope);

            continue;
        }

        if (!strcmp(tkn, "__spread"))
        {
            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_SPREAD, NULL, scope);

            rplni_prog_add(prog, &cmd, scope);

            continue;
        }

        if (!strcmp(tkn, "call"))
        {
            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_CALL, NULL, scope);

            rplni_prog_add(prog, &cmd, scope);

            continue;
        }

        if (!strcmp(tkn, "drop"))
        {
            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_DROP, NULL, scope);

            rplni_prog_add(prog, &cmd, scope);

            continue;
        }

        if (!strcmp(tkn, "dup"))
        {
            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_DUP, NULL, scope);

            rplni_prog_add(prog, &cmd, scope);

            continue;
        }

        if (!strcmp(tkn, "swap"))
        {
            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_SWAP, NULL, scope);

            rplni_prog_add(prog, &cmd, scope);

            continue;
        }

        if (!strcmp(tkn, "?!"))
        {
            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_ENDIF, NULL, scope);

            rplni_prog_add(prog, &cmd, scope);

            continue;
        }

        if (isdigit(*tkn))
        {
            struct rplni_value tmp;
            rplni_value_init_with_uint(&tmp, (uintptr_t)atol(tkn));

            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_PUSH, &tmp, scope);

            rplni_prog_add(prog, &cmd, scope);

            continue;
        }

        if (*tkn == '\"')
        {
            struct rplni_value tmp;
            rplni_value_init_with_cstr(&tmp, tkn + 1, scope);

            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, RPLNI_OP_PUSH, &tmp, scope);

            rplni_prog_add(prog, &cmd, scope);

            rplni_value_clean(&tmp, scope);

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
            else
            {
                if (!rplni_scope_has_var(scope, tkn))
                {
                    rplni_scope_add_var(scope, tkn);
                }

                rplni_value_init_with_cstr(&tmp, tkn, scope);
            }

            struct rplni_cmd cmd;
            rplni_cmd_init(&cmd, var_op, &tmp, scope);

            rplni_prog_add(prog, &cmd, scope);

            rplni_value_clean(&tmp, scope);

            continue;
        }

        fprintf(stderr, "unknown token [%s](len: %d)\n", tkn, (int)strlen(tkn));
    }

    rplni_loader_clean(&loader);

    return 1;
}

int rplni_func_load(
    struct rplni_func* func,
    size_t size, char* src,
    struct rplni_ptrlist* params, struct rplni_scope* scope,
    size_t* out_next)
{
    if (func == NULL || src == NULL || scope == NULL) return 0;

    struct rplni_loader loader;
    if (!rplni_loader_init(&loader)) return 0;

    size_t i = 0;
    for (; i < size;)
    {
        size_t tkn_start;
        size_t tkn_end;
        char* tkn;
        if (!rplni_loader_read(&loader, size - i, src + i,
            NULL, &tkn_end, &tkn, NULL))
        {
            break;
        }

        i += tkn_end;
        char* src2 = src + i;

        if (*tkn == '{' && tkn[1] == 0) break;

        rplni_func_add_param(func, tkn, 1);
    }

    size_t j;
    rplni_prog_load(&(func->prog),
        size - i, src + i,
        func->params,
        scope,
        &j);

    if (out_next != NULL) *out_next = i + j;
}
