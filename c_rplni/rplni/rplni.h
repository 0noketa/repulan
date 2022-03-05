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
    /* all objects allocated in this scope. */
    struct rul0i_set* objs;
    /* with type info */
    struct rul0i_set* strs;
    struct rul0i_set* lists;
};

enum rul0i_cmd_type
{
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
    RUL0I_FUNC_BLOCK,
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

struct rul0i_set
{
    uint32_t size;
    void **values;
};


struct rul0i_state
{
    struct rul0i_list *code_stack;
    struct rul0i_list *data_stack;
    struct rul0i_list *arg0_stack;
    struct rul0i_list *temp_stack;
};


/* values */
/* values themselves are not managed by GC */

int rul0i_value_init(struct rul0i_value *value);
int rul0i_value_ref(struct rul0i_value* value);  /* ref str/list/func */
int rul0i_value_clean(struct rul0i_value* value, struct rul0i_scope *scope);  /* unref str/list/func */
int rul0i_value_del(struct rul0i_value* value, struct rul0i_scope* scope);  /* similar to clean(), without ref check. */

/* does not touch refs */
int rul0i_values_init(size_t size, struct rul0i_value* values);
size_t rul0i_values_object_index(size_t size, struct rul0i_value* values, struct rul0i_value* value);
int rul0i_values_has_object(size_t size, struct rul0i_value* values, struct rul0i_value* value);
int rul0i_values_add_object(
    size_t size, struct rul0i_value* values,
    struct rul0i_value* value,
    struct rul0i_scope* scope,
    size_t* out_size, struct rul0i_value* out_values);
int rul0i_values_remove_object(size_t size, struct rul0i_value* values, struct rul0i_value* value);

int rul0i_named_init(struct rul0i_named *named, char *name, struct rul0i_scope* scope);
int rul0i_named_clean(struct rul0i_named *named, struct rul0i_scope *scope);

int rul0i_scope_init(struct rul0i_scope *scope);
int rul0i_scope_clean(struct rul0i_scope *scope);
void* rul0i_scope_malloc(struct rul0i_scope* scope, size_t size);
void* rul0i_scope_realloc(struct rul0i_scope* scope, void* p, size_t size);
int rul0i_scope_free(struct rul0i_scope* scope, void* p);
int rul0i_scope_add(struct rul0i_scope* scope, void* p);
int rul0i_scope_has(struct rul0i_scope* scope, void* p);
int rul0i_scope_remove(struct rul0i_scope* scope, void* p);
int rul0i_scope_add_str(struct rul0i_scope* scope, struct rul0i_str* p);
int rul0i_scope_has_str(struct rul0i_scope* scope, struct rul0i_str* p);
int rul0i_scope_remove_str(struct rul0i_scope* scope, struct rul0i_str* p);
int rul0i_scope_add_list(struct rul0i_scope* scope, struct rul0i_list* p);
int rul0i_scope_has_list(struct rul0i_scope* scope, struct rul0i_list* p);
int rul0i_scope_remove_list(struct rul0i_scope* scope, struct rul0i_list* p);
char* rul0i_scope_strdup(struct rul0i_scope* scope, char* s);

int rul0i_scope_remove_value(struct rul0i_scope* scope, struct rul0i_value* value);
int rul0i_scope_export_value(struct rul0i_scope* scope, struct rul0i_value* value, struct rul0i_scope* scope2);

int rul0i_scope_remove_str(struct rul0i_scope* scope, struct rul0i_str* str);
int rul0i_scope_export_str(struct rul0i_scope* scope, struct rul0i_str* str, struct rul0i_scope* scope2);

int rul0i_scope_remove_list(struct rul0i_scope* scope, struct rul0i_list* list);
int rul0i_scope_export_list(struct rul0i_scope* scope, struct rul0i_list* list, struct rul0i_scope* scope2);


int rul0i_state_init(struct rul0i_state* state, struct rul0i_scope* scope);
int rul0i_state_clean(struct rul0i_state *state, struct rul0i_scope *scope);


/* objects */
/* objects will be deallocated by GC */

struct rul0i_str *rul0i_str_new(char *value, struct rul0i_scope* scope);
int rul0i_str_init(struct rul0i_str *str, char *value, struct rul0i_scope* scope);
int rul0i_str_ref(struct rul0i_str *str);
int rul0i_str_unref(struct rul0i_str* str, struct rul0i_scope* scope);
int rul0i_str_del(struct rul0i_str* str, struct rul0i_scope* scope);
int rul0i_str_add(struct rul0i_str* str, struct rul0i_str* str2, struct rul0i_scope* scope);
int rul0i_str_add_cstr(struct rul0i_str* str, char* str2, struct rul0i_scope* scope);

struct rul0i_list *rul0i_list_new(size_t cap, struct rul0i_scope* scope);
int rul0i_list_init(struct rul0i_list* list, size_t cap, struct rul0i_scope* scope);
struct rul0i_list* rul0i_list_new_with_captured(size_t size, struct rul0i_list* stack, struct rul0i_scope* scope);
int rul0i_list_init_with_captured(struct rul0i_list* list, size_t size, struct rul0i_list* stack, struct rul0i_scope* scope);
int rul0i_list_ref(struct rul0i_list *list);
/* deallocates when refs was 0 */
int rul0i_list_unref(struct rul0i_list *list, struct rul0i_scope* scope);
/* deallocates wiithout ref checking. exported objects will not be deallocated. */
int rul0i_list_del(struct rul0i_list* list, struct rul0i_scope* scope);
int rul0i_list_push(struct rul0i_list *list, struct rul0i_value *value, struct rul0i_scope* scope);
int rul0i_list_pop(struct rul0i_list *list, struct rul0i_value *out_value, struct rul0i_scope* scope);

/* moved objects will not be deallocated by old scope's clean(). */
/* currently scope-tangled objects like below will cause memory-corruption.
   export/import at assignment is required.
     X(scope:A) --> Y(scope:B) --> Z(scope:A) --> X
*/
int rul0i_scope_export_value(struct rul0i_scope* scope, struct rul0i_value* value, struct rul0i_scope* scope2);
int rul0i_scope_export_str(struct rul0i_scope* scope, struct rul0i_str* str, struct rul0i_scope* scope2);
int rul0i_scope_export_list(struct rul0i_scope* scope, struct rul0i_list* list, struct rul0i_scope* scope2);


/* array list for any pointer */
struct rul0i_set *rul0i_set_new(void);
int rul0i_set_init(struct rul0i_set *nodes);
int rul0i_set_del(struct rul0i_set *nodes);
size_t rul0i_set_index(struct rul0i_set* nodes, void* node);
int rul0i_set_has(struct rul0i_set *nodes, void *node);
int rul0i_set_add(struct rul0i_set *nodes, void *node);
int rul0i_set_remove(struct rul0i_set* nodes, void* node);


#endif
