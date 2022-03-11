#ifndef rplni_macro__h
#define rplni_macro__h

#include "rplni.h"


#define RPLNI_DEF(name) struct rplni_value name; rplni_value_init(&name)
#define RPLNI_DEF_UINT(name, u) struct rplni_value name; rplni_value_init_with_uint(&name, u);
#define RPLNI_DEF_STR(name, s, scope) struct rplni_value name; rplni_value_init_with_cstr(&name, s, scope);
#define RPLNI_DEF_FUNC(name, type, scope) struct rplni_value name; rplni_value_init_with_empty_func(&name, type, scope);

#define RPLNI_CMD_(name, op, arg) \
    struct rplni_cmd name; \
    rplni_cmd_init(&name, op, arg, scope)
#define CODE_I(op, arg) { \
    RPLNI_DEF_UINT(tmp, arg); \
    RPLNI_CMD_(tmp2, op, &tmp); \
    rplni_prog_add(&prog, &tmp2, scope); \
    rplni_value_clean(&tmp, scope); \
}0
#define CODE_S(op, arg) { \
    RPLNI_DEF_STR(tmp, arg, scope); \
    RPLNI_CMD_(tmp2, op, &tmp); \
    rplni_prog_add(&prog, &tmp2, scope); \
    rplni_value_clean(&tmp, scope); \
}0
#define CODE_V(op, arg) { \
    RPLNI_CMD_(tmp, op, arg); \
    rplni_prog_add(&prog, &tmp, scope); \
}0


#endif
