#ifndef rplni__h
#define rplni__h

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifndef RPLNI_DATA_STACK_DEFAULT_CAP
#define RPLNI_DATA_STACK_DEFAULT_CAP 0x400
#endif
#ifndef RPLNI_ARG0_STACK_DEFAULT_CAP
#define RPLNI_ARG0_STACK_DEFAULT_CAP 0x100
#endif
#ifndef RPLNI_TEMP_STACK_DEFAULT_CAP
#define RPLNI_TEMP_STACK_DEFAULT_CAP 0x100
#endif

#define RPLNI_LIST_BLOCK_SIZE 32

/* satulated number of elements */
#define RPLNI_LIST_CEILED_SIZE(n)  (((n) / RPLNI_LIST_BLOCK_SIZE + 1) * RPLNI_LIST_BLOCK_SIZE)


enum rplni_value_type
{
    RPLNI_UINT,
    RPLNI_STR,
    RPLNI_LIST,
    RPLNI_BUILTIN,
    RPLNI_FUNC,
};

struct rplni_str
{
    int refs;
    size_t size;
    char *value;
};

struct rplni_list
{
    int refs;
    size_t cap;
    size_t size;
    struct rplni_value *values;
};

struct rplni_value
{
    enum rplni_value_type type;
    union {
        uintptr_t _uint;
        struct rplni_func *_func;
        struct rplni_list *_list;
        struct rplni_str *_str;
    } value;
};

struct rplni_named
{
    char *name;
    struct rplni_value value;
};

struct rplni_scope
{
    size_t size;
    struct rplni_named *vars;
    /* all objects allocated in this scope. */
    struct rplni_set* objs;
    /* with type info */
    struct rplni_set* strs;
    struct rplni_set* lists;
};

enum rplni_cmd_type
{
    RPLNI_OP_PUSH,
    RPLNI_OP_POP,
    RPLNI_OP_ADD,
    RPLNI_OP_SUB,
    RPLNI_OP_DIV,
    RPLNI_OP_MOD,
    RPLNI_OP_BEGIN_ARGS,
    RPLNI_OP_END_ARGS,
    RPLNI_OP_BEGIN_LIST,
    RPLNI_OP_END_LIST,
    RPLNI_OP_IF,
    RPLNI_OP_ELSE,
    RPLNI_OP_ENDIF,
    RPLNI_OP_RESTART,
};

struct rplni_cmd
{
    enum rplni_cmd_type type;
    struct rplni_value arg;
};

struct rplni_prog
{
    size_t size;
    struct rplni_cmd *code;
};

enum rplni_func_type
{
    RPLNI_FUNC_BUILTIN,
    RPLNI_FUNC_BLOCK,
    RPLNI_FUNC_LAMBDA,
    RPLNI_FUNC_CLOSURE,
    RPLNI_FUNC_FOR,
    RPLNI_FUNC_CLOSURE_FOR
};

struct rplni_func
{
    int refs;
    enum rplni_func_type type;
    struct rplni_scope scope;
    struct rplni_prog prog;
};

struct rplni_set
{
    uint32_t size;
    void **values;
};


struct rplni_state
{
    struct rplni_list *code_stack;
    struct rplni_list *data_stack;
    struct rplni_list *arg0_stack;
    struct rplni_list *temp_stack;
};


/* values */
/* values themselves are not managed by GC */

int rplni_value_init(struct rplni_value *value);
int rplni_value_ref(struct rplni_value* value);  /* ref str/list/func */
int rplni_value_clean(struct rplni_value* value, struct rplni_scope *scope);  /* unref str/list/func */
int rplni_value_del(struct rplni_value* value, struct rplni_scope* scope);  /* similar to clean(), without ref check. */

/* does not touch refs */
int rplni_values_init(size_t size, struct rplni_value* values);
size_t rplni_values_object_index(size_t size, struct rplni_value* values, struct rplni_value* value);
int rplni_values_has_object(size_t size, struct rplni_value* values, struct rplni_value* value);
int rplni_values_add_object(
    size_t size, struct rplni_value* values,
    struct rplni_value* value,
    struct rplni_scope* scope,
    size_t* out_size, struct rplni_value* out_values);
int rplni_values_remove_object(size_t size, struct rplni_value* values, struct rplni_value* value);

int rplni_named_init(struct rplni_named *named, char *name, struct rplni_scope* scope);
int rplni_named_clean(struct rplni_named *named, struct rplni_scope *scope);

int rplni_scope_init(struct rplni_scope *scope);
int rplni_scope_clean(struct rplni_scope *scope);
void* rplni_scope_malloc(struct rplni_scope* scope, size_t size);
void* rplni_scope_realloc(struct rplni_scope* scope, void* p, size_t size);
int rplni_scope_free(struct rplni_scope* scope, void* p);
int rplni_scope_add(struct rplni_scope* scope, void* p);
int rplni_scope_has(struct rplni_scope* scope, void* p);
int rplni_scope_remove(struct rplni_scope* scope, void* p);
int rplni_scope_add_str(struct rplni_scope* scope, struct rplni_str* p);
int rplni_scope_has_str(struct rplni_scope* scope, struct rplni_str* p);
int rplni_scope_remove_str(struct rplni_scope* scope, struct rplni_str* p);
int rplni_scope_add_list(struct rplni_scope* scope, struct rplni_list* p);
int rplni_scope_has_list(struct rplni_scope* scope, struct rplni_list* p);
int rplni_scope_remove_list(struct rplni_scope* scope, struct rplni_list* p);
char* rplni_scope_strdup(struct rplni_scope* scope, char* s);

int rplni_scope_remove_value(struct rplni_scope* scope, struct rplni_value* value);
int rplni_scope_export_value(struct rplni_scope* scope, struct rplni_value* value, struct rplni_scope* scope2);

int rplni_scope_remove_str(struct rplni_scope* scope, struct rplni_str* str);
int rplni_scope_export_str(struct rplni_scope* scope, struct rplni_str* str, struct rplni_scope* scope2);

int rplni_scope_remove_list(struct rplni_scope* scope, struct rplni_list* list);
int rplni_scope_export_list(struct rplni_scope* scope, struct rplni_list* list, struct rplni_scope* scope2);


int rplni_state_init(struct rplni_state* state, struct rplni_scope* scope);
int rplni_state_clean(struct rplni_state *state, struct rplni_scope *scope);


/* objects */
/* objects will be deallocated by GC */

struct rplni_str *rplni_str_new(char *value, struct rplni_scope* scope);
int rplni_str_init(struct rplni_str *str, char *value, struct rplni_scope* scope);
int rplni_str_ref(struct rplni_str *str);
int rplni_str_unref(struct rplni_str* str, struct rplni_scope* scope);
int rplni_str_del(struct rplni_str* str, struct rplni_scope* scope);
int rplni_str_add(struct rplni_str* str, struct rplni_str* str2, struct rplni_scope* scope);
int rplni_str_add_cstr(struct rplni_str* str, char* str2, struct rplni_scope* scope);

struct rplni_list *rplni_list_new(size_t cap, struct rplni_scope* scope);
int rplni_list_init(struct rplni_list* list, size_t cap, struct rplni_scope* scope);
struct rplni_list* rplni_list_new_with_captured(size_t size, struct rplni_list* stack, struct rplni_scope* scope);
int rplni_list_init_with_captured(struct rplni_list* list, size_t size, struct rplni_list* stack, struct rplni_scope* scope);
int rplni_list_ref(struct rplni_list *list);
/* deallocates when refs was 0 */
int rplni_list_unref(struct rplni_list *list, struct rplni_scope* scope);
/* deallocates wiithout ref checking. exported objects will not be deallocated. */
int rplni_list_del(struct rplni_list* list, struct rplni_scope* scope);
int rplni_list_push(struct rplni_list *list, struct rplni_value *value, struct rplni_scope* scope);
int rplni_list_pop(struct rplni_list *list, struct rplni_value *out_value, struct rplni_scope* scope);

/* moved objects will not be deallocated by old scope's clean(). */
/* currently scope-tangled objects like below will cause memory-corruption.
   export/import at assignment is required.
     X(scope:A) --> Y(scope:B) --> Z(scope:A) --> X
*/
int rplni_scope_export_value(struct rplni_scope* scope, struct rplni_value* value, struct rplni_scope* scope2);
int rplni_scope_export_str(struct rplni_scope* scope, struct rplni_str* str, struct rplni_scope* scope2);
int rplni_scope_export_list(struct rplni_scope* scope, struct rplni_list* list, struct rplni_scope* scope2);


/* array list for any pointer */
struct rplni_set *rplni_set_new(void);
int rplni_set_init(struct rplni_set *nodes);
int rplni_set_del(struct rplni_set *nodes);
size_t rplni_set_index(struct rplni_set* nodes, void* node);
int rplni_set_has(struct rplni_set *nodes, void *node);
int rplni_set_add(struct rplni_set *nodes, void *node);
int rplni_set_remove(struct rplni_set* nodes, void* node);


#endif
