#ifndef rul0i__h
#define rul0i__h

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifndef RUL0I_DATA_STACK_DEFAULT_CAP
#define RUL0I_DATA_STACK_DEFAULT_CAP 0x400
#endif
#ifndef RUL0I_ARG0_STACK_DEFAULT_CAP
#define RUL0I_ARG0_STACK_DEFAULT_CAP 0x100
#endif
#ifndef RUL0I_TEMP_STACK_DEFAULT_CAP
#define RUL0I_TEMP_STACK_DEFAULT_CAP 0x100
#endif

#define RUL0I_LIST_BLOCK_SIZE 32

/* satulated number of elements */
#define RUL0I_LIST_CEILED_SIZE(n)  (((n) / RUL0I_LIST_BLOCK_SIZE + 1) * RUL0I_LIST_BLOCK_SIZE)


enum rul0i_value_type
{
    RUL0I_UINT,
    RUL0I_STR,
    RUL0I_LIST,
    RUL0I_BUILTIN,
    RUL0I_FUNC,
};

struct rul0i_str
{
    int refs;
    size_t size;
    char *value;
};

struct rul0i_list
{
    int refs;
    size_t cap;
    size_t size;
    struct rul0i_value *values;
};

struct rul0i_value
{
    enum rul0i_value_type type;
    union {
        uintptr_t _uint;
        struct rul0i_func *_func;
        struct rul0i_list *_list;
        struct rul0i_str *_str;
    } value;
};

struct rul0i_named
{
    char *name;
    struct rul0i_value value;
};

struct rul0i_scope
{
    size_t size;
    struct rul0i_named *vars;
};

enum rul0i_cmd_type {
    RUL0I_OP_PUSH,
    RUL0I_OP_POP,
    RUL0I_OP_ADD,
    RUL0I_OP_SUB,
    RUL0I_OP_DIV,
    RUL0I_OP_MOD,
    RUL0I_OP_BEGIN_ARGS,
    RUL0I_OP_END_ARGS,
    RUL0I_OP_BEGIN_LIST,
    RUL0I_OP_END_LIST,
    RUL0I_OP_IF,
    RUL0I_OP_ELSE,
    RUL0I_OP_ENDIF,
    RUL0I_OP_RESTART,
};

struct rul0i_cmd
{
    enum rul0i_cmd_type type;
    struct rul0i_value arg;
};

struct rul0i_prog
{
    size_t size;
    struct rul0i_cmd *code;
};

enum rul0i_func_type
{
    RUL0I_FUNC_BUILTIN,
    RUL0I_FUNC_LAMBDA,
    RUL0I_FUNC_CLOSURE,
    RUL0I_FUNC_FOR,
    RUL0I_FUNC_CLOSURE_FOR
};

struct rul0i_func
{
    int refs;
    enum rul0i_func_type type;
    struct rul0i_scope scope;
    struct rul0i_prog prog;
};

struct rul0i_gc_nodes
{
    uint32_t size;
    void **nodes;
};


struct rul0i_stack
{
    size_t size;
    size_t top; 
    struct rul0i_value *stack;
};


struct rul0i_state
{
    struct rul0i_scope global_scope;
    struct rul0i_list *data_stack;
    struct rul0i_list *arg0_stack;
    struct rul0i_list *temp_stack;
};


/* values */

int rul0i_value_init(struct rul0i_value *value);
int rul0i_value_ref(struct rul0i_value* value);  /* ref str/list/func, not value's */
int rul0i_value_clean(struct rul0i_value* value, struct rul0i_gc_nodes* nodes);  /* unref str/list/func, not value's */
int rul0i_named_init(struct rul0i_named *named, char *name);
int rul0i_named_clean(struct rul0i_named *named, struct rul0i_gc_nodes *nodes);
int rul0i_scope_init(struct rul0i_scope *scope);
int rul0i_scope_clean(struct rul0i_scope *scope, struct rul0i_gc_nodes *nodes);
int rul0i_scope_add(struct rul0i_scope *scope, struct rul0i_value *value);
int rul0i_state_init(struct rul0i_state *state);
int rul0i_state_clean(struct rul0i_state *state, struct rul0i_gc_nodes *nodes);


/* objects */

struct rul0i_str *rul0i_str_new(char *value);
int rul0i_str_init(struct rul0i_str *str, char *value);
int rul0i_str_ref(struct rul0i_str *str);
int rul0i_str_unref(struct rul0i_str *str, struct rul0i_gc_nodes *nodes);
int rul0i_str_add(struct rul0i_str* str, struct rul0i_str* str2);
int rul0i_str_add_cstr(struct rul0i_str* str, char* str2);

struct rul0i_list *rul0i_list_new(size_t cap);
int rul0i_list_init(struct rul0i_list* list, size_t cap);
int rul0i_list_ref(struct rul0i_list *list);
/* deallocates when refs was 0 */
int rul0i_list_unref(struct rul0i_list *list, struct rul0i_gc_nodes *nodes);
int rul0i_list_push(struct rul0i_list *list, struct rul0i_value *value);
int rul0i_list_pop(struct rul0i_list *list, struct rul0i_value *out_value);



struct rul0i_gc_nodes *rul0i_gc_nodes_new(void);
int rul0i_gc_nodes_init(struct rul0i_gc_nodes *nodes);
int rul0i_gc_nodes_clean(struct rul0i_gc_nodes *nodes);
int rul0i_gc_nodes_has(struct rul0i_gc_nodes *nodes, void *node);
int rul0i_gc_nodes_add(struct rul0i_gc_nodes *nodes, void *node);


#endif
