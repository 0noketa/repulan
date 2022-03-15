#ifndef rplni__h
#define rplni__h

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#ifndef RPLNI_DATA_STACK_DEFAULT_CAP
#define RPLNI_DATA_STACK_DEFAULT_CAP 0x400
#endif
#ifndef RPLNI_ARG0_STACK_DEFAULT_CAP
#define RPLNI_ARG0_STACK_DEFAULT_CAP 0x100
#endif
#ifndef RPLNI_CALLEE_STACK_DEFAULT_CAP
#define RPLNI_CALLEE_STACK_DEFAULT_CAP 0x10
#endif
#ifndef RPLNI_TEMP_STACK_DEFAULT_CAP
#define RPLNI_TEMP_STACK_DEFAULT_CAP 0x100
#endif

#define RPLNI_LIST_BLOCK_SIZE 32
#define RPLNI_REFS_MAX INT_MAX

/* number of elements */
#define RPLNI_LIST_CEILED_SIZE(n)  (((n) / RPLNI_LIST_BLOCK_SIZE + 1) * RPLNI_LIST_BLOCK_SIZE)


enum rplni_value_type
{
    RPLNI_UINT,
    RPLNI_STR,
    RPLNI_LIST,
    RPLNI_BUILTIN,
    RPLNI_FUNC,
    RPLNI_CLOSURE,
};

struct rplni_str
{
    int refs;
    struct rplni_scope* owner;
    size_t size;
    char *value;
};

struct rplni_list
{
    int refs;
    struct rplni_scope* owner;
    size_t cap;
    size_t size;
    struct rplni_value *values;
};

struct rplni_value
{
    enum rplni_value_type type;
    union {
        uintptr_t _uint;
        struct rplni_func* _func;
        struct rplni_closure* _closure;
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
    size_t cap;
    size_t size;
    struct rplni_named *vars;
    /* all objects allocated in this scope. */
    struct rplni_ptrlist* objs;
    /* typed */
    struct rplni_ptrlist* strs;
    struct rplni_ptrlist* lists;
    struct rplni_ptrlist* funcs;

    struct rplni_ptrlist* deallocation_history;
};

enum rplni_op_type
{
    RPLNI_OP_NOP,
    RPLNI_OP_PUSH,
    RPLNI_OP_POP,
    RPLNI_OP_EXPORT,
    RPLNI_OP_LOAD,
    RPLNI_OP_STORE,
    RPLNI_OP_DUP,
    RPLNI_OP_SWAP,
    RPLNI_OP_DROP,
    RPLNI_OP_ADD,
    RPLNI_OP_SUB,
    RPLNI_OP_MUL,
    RPLNI_OP_DIV,
    RPLNI_OP_MOD,
    RPLNI_OP_RANGE,
    RPLNI_OP_BEGIN_ARGS,
    RPLNI_OP_END_ARGS,
    RPLNI_OP_BEGIN_LIST,
    RPLNI_OP_END_LIST,
    RPLNI_OP_IF,  /* until JIT optimization, this instruction does not know position of ELSE and ENDIF */
    RPLNI_OP_ELSE,
    RPLNI_OP_ENDIF,
    RPLNI_OP_GOTO,
    RPLNI_OP_CALL,
    RPLNI_OP_RESTART,
    RPLNI_OP_LEN,
    RPLNI_OP_LIST_PUSH,
    RPLNI_OP_LIST_POP,
    RPLNI_OP_SPREAD,
    RPLNI_OP_PRINT,
    RPLNI_OP_INPUT,
};

struct rplni_cmd
{
    enum rplni_op_type op;
    struct rplni_value arg;
};

struct rplni_prog
{
    size_t cap;
    size_t size;
    struct rplni_cmd *code;
};

enum rplni_func_type
{
    RPLNI_FUNC_BUILTIN,
    RPLNI_FUNC_BLOCK,
    RPLNI_FUNC_FUNC,
    RPLNI_FUNC_CLOSURE,
    RPLNI_FUNC_FOR,
    RPLNI_FUNC_CLOSURE_FOR
};

struct rplni_func
{
    int refs;
    struct rplni_scope* owner;
    enum rplni_func_type type;
    struct rplni_prog prog;
    /* strlist */
    struct rplni_ptrlist* params;
    /* strlist */
    struct rplni_ptrlist* members;
};

struct rplni_closure
{
    int refs;
    struct rplni_scope *owner;
    struct rplni_func *funcdef;
    struct rplni_scope scope;
};

struct rplni_ptrlist
{
    size_t size_;
    int is_strlist;
    union {
        void** any;
        struct rplni_str** str;
        struct rplni_list** list;
        struct rplni_func** func;
        char** cstr;
    } values;
};


struct rplni_state
{
    struct rplni_scope scope;
    struct rplni_list* data_stack;
    struct rplni_list* arg0_stack;
    struct rplni_list* callee_stack;
    struct rplni_list* list_head_stack;
    struct rplni_list* temp_stack;
    struct rplni_ptrlist* scope_stack;
};


/* usage of unref & del
    if scope was NULL, deallocation starts with empty history.
    if scope was not NULL, deallocation uses old history. this is necessary for unconditional deallocation.
*/

/* values */
/* values themselves are not managed by GC */

int rplni_value_init(struct rplni_value *value);
int rplni_value_init_with_uint(struct rplni_value* value, uintptr_t uint_value);
int rplni_value_init_with_cstr(struct rplni_value* value, char* str_value, struct rplni_scope* scope);
int rplni_value_init_with_empty_func(struct rplni_value* value, enum rplni_func_type type, struct rplni_scope* scope);
int rplni_value_ref(struct rplni_value* value);  /* ref str/list/func */
int rplni_value_clean(struct rplni_value* value, struct rplni_scope* scope);  /* unref str/list/func */
int rplni_value_del(struct rplni_value* value, struct rplni_scope* scope);  /* similar to clean(), without ref check. */
int rplni_value_owner(struct rplni_value* value, struct rplni_scope** out_owner);

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

/* when arg == NULL, operand becomes uint(0) */
int rplni_cmd_init(struct rplni_cmd* cmd, enum rplni_op_type op, struct rplni_value* arg, struct rplni_scope* scope);
int rplni_cmd_clean(struct rplni_cmd* cmd, struct rplni_scope* scope);
int rplni_cmd_del(struct rplni_cmd* cmd, struct rplni_scope* scope);
int rplni_prog_init(struct rplni_prog* prog, struct rplni_scope* scope);
int rplni_prog_add(struct rplni_prog* prog, struct rplni_cmd* cmd, struct rplni_scope* scope);
int rplni_prog_clean(struct rplni_prog* prog, struct rplni_scope* owner, struct rplni_scope* scope);
int rplni_prog_del(struct rplni_prog* prog, struct rplni_scope* owner, struct rplni_scope* scope);
int rplni_prog_run(struct rplni_prog* prog, struct rplni_state* state);

int rplni_scope_init(struct rplni_scope *scope);
int rplni_scope_clean(struct rplni_scope *scope);
int rplni_scope_add_var(struct rplni_scope* scope, char* name);
size_t rplni_scope_var_index(struct rplni_scope* scope, char* name);
int rplni_scope_has_var(struct rplni_scope* scope, char* name);
int rplni_scope_load_var(struct rplni_scope* scope, char* name, struct rplni_value* out_value);
int rplni_scope_store_var(struct rplni_scope* scope, char* name, struct rplni_value* value);
int rplni_scope_load_var_by_index(struct rplni_scope* scope, size_t idx, struct rplni_value* out_value);
int rplni_scope_store_var_by_index(struct rplni_scope* scope, size_t idx, struct rplni_value* value);

void* rplni_scope_malloc(struct rplni_scope* scope, size_t size);
void* rplni_scope_realloc(struct rplni_scope* scope, void* p, size_t size);
int rplni_scope_free(struct rplni_scope* scope, void* p);
int rplni_scope_add(struct rplni_scope* scope, void* p);
int rplni_scope_has(struct rplni_scope* scope, void* p);
int rplni_scope_remove(struct rplni_scope* scope, void* p);
int rplni_scope_add_value(struct rplni_scope* scope, struct rplni_value* p);
int rplni_scope_has_value(struct rplni_scope* scope, struct rplni_value* p);
int rplni_scope_remove_value(struct rplni_scope* scope, struct rplni_value* p);
int rplni_scope_add_str(struct rplni_scope* scope, struct rplni_str* p);
int rplni_scope_has_str(struct rplni_scope* scope, struct rplni_str* p);
int rplni_scope_remove_str(struct rplni_scope* scope, struct rplni_str* p);
int rplni_scope_add_list(struct rplni_scope* scope, struct rplni_list* p);
int rplni_scope_has_list(struct rplni_scope* scope, struct rplni_list* p);
int rplni_scope_remove_list(struct rplni_scope* scope, struct rplni_list* p);
int rplni_scope_add_func(struct rplni_scope* scope, struct rplni_func* p);
int rplni_scope_has_func(struct rplni_scope* scope, struct rplni_func* p);
int rplni_scope_remove_func(struct rplni_scope* scope, struct rplni_func* p);
char* rplni_scope_strdup(struct rplni_scope* scope, char* s);

/* moved objects will not be deallocated by old scope's clean(). */
/* currently scope-tangled objects like below will cause memory-corruption.
   export/import at assignment is required.
     X(scope:A) --> Y(scope:B) --> Z(scope:A) --> X
*/
int rplni_scope_export_value(struct rplni_scope* scope, struct rplni_value* value, struct rplni_scope* scope2);
int rplni_scope_export_str(struct rplni_scope* scope, struct rplni_str* str, struct rplni_scope* scope2);
int rplni_scope_export_list(struct rplni_scope* scope, struct rplni_list* list, struct rplni_scope* scope2);
int rplni_scope_export_prog(struct rplni_scope* scope, struct rplni_prog* prog, struct rplni_scope* scope2);
int rplni_scope_export_func(struct rplni_scope* scope, struct rplni_func* func, struct rplni_scope* scope2);

int rplni_state_init(struct rplni_state* state);
int rplni_state_clean(struct rplni_state *state);
int rplni_state_current_scope(struct rplni_state* state, struct rplni_scope** out_scope);
int rplni_state_outer_scope(struct rplni_state* state, struct rplni_scope** out_scope);
int rplni_state_push_scope(struct rplni_state* state, struct rplni_scope* scope);
int rplni_state_pop_scope(struct rplni_state* state, struct rplni_scope** out_scope);
int rplni_state_find_var(struct rplni_state* state, char *name, struct rplni_scope** out_scope, size_t *out_index);
int rplni_state_load_var(struct rplni_state* state, char* name, struct rplni_value* out_value);
int rplni_state_store_var(struct rplni_state* state, char* name, struct rplni_value* value);
int rplni_state_has_scope(struct rplni_state* state, struct rplni_scope* scope);
int rplni_state_compare_scopes(struct rplni_state* state, struct rplni_scope* scope, struct rplni_scope* scope2, int* out_result);
int rplni_state_push_value(struct rplni_state* state, struct rplni_value* value);
int rplni_state_pop_value(struct rplni_state* state, struct rplni_value* out_value);
int rplni_state_gather_values(struct rplni_state* state, struct rplni_value* value, struct rplni_value* value2);

/* objects */
/* objects will be deallocated by GC */

struct rplni_str *rplni_str_new(char *value, struct rplni_scope* scope);
int rplni_str_init(struct rplni_str *str, char *value, struct rplni_scope* scope);
int rplni_str_ref(struct rplni_str *str);
int rplni_str_unref(struct rplni_str* str, struct rplni_scope* scope);
int rplni_str_del(struct rplni_str* str, struct rplni_scope* scope);
int rplni_str_add(struct rplni_str* str, struct rplni_str* str2);
int rplni_str_add_cstr(struct rplni_str* str, char* str2);

struct rplni_list *rplni_list_new(size_t cap, struct rplni_scope* scope);
int rplni_list_init(struct rplni_list* list, size_t cap, struct rplni_scope* scope);
struct rplni_list* rplni_list_new_with_captured(size_t size, struct rplni_list* stack, struct rplni_scope* scope);
int rplni_list_init_with_captured(struct rplni_list* list, size_t size, struct rplni_list* stack, struct rplni_scope* scope);
int rplni_list_ref(struct rplni_list *list);
/* deallocates when refs was 0 */
int rplni_list_unref(struct rplni_list *list, struct rplni_scope* scope);
/* deallocates wiithout ref checking. exported objects will not be deallocated. */
int rplni_list_del(struct rplni_list* list, struct rplni_scope* scope);
int rplni_list_push(struct rplni_list *list, struct rplni_value *value);
int rplni_list_pop(struct rplni_list *list, struct rplni_value *out_value);

struct rplni_func* rplni_func_new(enum rplni_func_type type, struct rplni_scope* scope);
int rplni_func_init(struct rplni_func* func, enum rplni_func_type type, struct rplni_scope* scope);
int rplni_func_ref(struct rplni_func* func);
int rplni_func_unref(struct rplni_func* func, struct rplni_scope* scope);
int rplni_func_del(struct rplni_func* func, struct rplni_scope* scope);
int rplni_func_add_param(struct rplni_func* func, char* name, int copy_str);
int rplni_func_run(struct rplni_func* func, struct rplni_state* state);


/* array list for non-null unique pointers */
struct rplni_ptrlist *rplni_ptrlist_new(void);
/* initialized as strlist that deallocates elements in del() */
struct rplni_ptrlist* rplni_ptrlist_new_as_strlist();
int rplni_ptrlist_init(struct rplni_ptrlist *nodes, int as_strlist);
int rplni_ptrlist_del(struct rplni_ptrlist *nodes);
size_t rplni_ptrlist_index(struct rplni_ptrlist* nodes, void* node);
int rplni_ptrlist_has(struct rplni_ptrlist *nodes, void *node);
int rplni_ptrlist_add(struct rplni_ptrlist *nodes, void *node);
int rplni_ptrlist_add_str(struct rplni_ptrlist *nodes, char *node, int copy_value);
int rplni_ptrlist_remove(struct rplni_ptrlist* nodes, void* node);
/* returns number of elements. ".size_" of this struct does not mean it. */
size_t rplni_ptrlist_len(struct rplni_ptrlist* list);
int rplni_ptrlist_push(struct rplni_ptrlist* nodes, void* node);
int rplni_ptrlist_push_str(struct rplni_ptrlist* nodes, char* value, int copy_value);
int rplni_ptrlist_pop(struct rplni_ptrlist* nodes, void** out_node);
int rplni_ptrlist_clear(struct rplni_ptrlist *nodes);


#ifndef NDEBUG
void rplni_dump_leaked(void);
#endif


#endif
