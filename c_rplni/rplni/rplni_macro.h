#ifndef rplni_macro__h
#define rplni_macro__h

#include "rplni.h"


#define RPLNI_DEF(name) struct rplni_value name; rplni_value_init(&name)
#define RPLNI_DEF_UINT(name, u) struct rplni_value name; rplni_value_init_with_uint(&name, u);
#define RPLNI_DEF_STR(name, s, state) struct rplni_value name; rplni_value_init_with_cstr(&name, strlen(s), s, state);
#define RPLNI_DEF_FUNC(name, type, state) struct rplni_value name; rplni_value_init_with_empty_func(&name, type, state);

#define RPLNI_CMD_(name, op, arg) \
    struct rplni_cmd name; \
    rplni_cmd_init(&name, op, arg, state)
#define CODE_I(op, arg) { \
    RPLNI_DEF_UINT(tmp, arg); \
    RPLNI_CMD_(tmp2, op, &tmp); \
    rplni_prog_add(&prog, &tmp2, state); \
    rplni_value_clean(&tmp, NULL); \
}0
#define CODE_S(op, arg) { \
    RPLNI_DEF_STR(tmp, arg, state); \
    RPLNI_CMD_(tmp2, op, &tmp); \
    rplni_prog_add(&prog, &tmp2, state); \
    rplni_value_clean(&tmp, NULL); \
}0
#define CODE_V(op, arg) { \
    RPLNI_CMD_(tmp, op, arg); \
    rplni_prog_add(&prog, &tmp, state); \
}0


#endif
