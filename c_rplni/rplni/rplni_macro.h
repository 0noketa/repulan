#ifndef rplni_macro__h
#define rplni_macro__h

#include "rplni.h"


#define RPLNI_DEF(name) struct rplni_value name; rplni_value_init(&name)
#define RPLNI_UNDEF(name) rplni_value_clean(&name, NULL)


#endif
