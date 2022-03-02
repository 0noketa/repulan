#ifndef rul0i_macros__h
#define rul0i_macros__h

#include "rul0i.h"


#define RUL0I_DEF(name) struct rul0i_value name; rul0i_value_init(&name)
#define RUL0I_UNDEF(name) rul0i_value_clean(&name, NULL)


#endif
