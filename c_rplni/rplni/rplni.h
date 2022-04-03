#ifndef rplni__h
#define rplni__h

#include <limits.h>
#include <stdint.h>
#include <inttypes.h>
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
#ifndef RPLNI_LIST_HEAD_STACK_DEFAULT_CAP
#define RPLNI_LIST_HEAD_STACK_DEFAULT_CAP 0x80
#endif

#define RPLNI_LIST_BLOCK_SIZE 32
#define RPLNI_TMPSTR_PADDING_SIZE 32
/* number of calls */
#define RPLNI_GC_DEFAULT_INTERVAL 0x1000

/* number of elements */
#define RPLNI_LIST_CEILED_SIZE(n)  (((n) / RPLNI_LIST_BLOCK_SIZE + 1) * RPLNI_LIST_BLOCK_SIZE)


enum rplni_value_type
{
    RPLNI_TYPE_UINT,
    RPLNI_TYPE_SYM,
    RPLNI_TYPE_STR,
    RPLNI_TYPE_LIST,
    RPLNI_TYPE_FUNC,
    RPLNI_TYPE_CLOSURE,
    RPLNI_TYPE_SLICE,
    RPLNI_TYPE_CFUNC,
    RPLNI_TYPE_CDATA,
};

typedef uint_fast16_t rplni_id_t;
#define RPLNI_ID_MAX UINT16_MAX

struct rplni_str
{
    struct rplni_state* owner;
    size_t size;
    char *value;
};

struct rplni_list
{
    struct rplni_state* owner;
    size_t cap;
    size_t size;
    struct rplni_value *values;
};

typedef int (*rplni_cfunc)(struct rplni_state* state);
typedef int (*rplni_cdata_destructor)(struct rplni_state* state, void *data);

struct rplni_cdata
{
    struct rplni_state* owner;
    void* data;
    rplni_cdata_destructor del;
};


struct rplni_value
{
    enum rplni_value_type type;
    union {
        uintptr_t _uint;
        rplni_id_t _sym;
        struct rplni_func* _func;
        struct rplni_closure* _closure;
        struct rplni_list *_list;
        struct rplni_str* _str;
        struct rplni_slice* _iter;
        rplni_cfunc _cfunc;
        struct rplni_cdata* _cdata;
    } value;
};

struct rplni_slice
{
    struct rplni_state* owner;
    size_t start;
    size_t end;
    struct rplni_value callable;
};

struct rplni_named
{
    rplni_id_t id;
    struct rplni_value value;
};

struct rplni_scope
{
    struct rplni_state* owner;
    size_t cap;
    size_t size;
    struct rplni_named *vars;
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
    RPLNI_OP_EQ,
    RPLNI_OP_NEQ,
    RPLNI_OP_LT,
    RPLNI_OP_GT,
    RPLNI_OP_RANGE,
    RPLNI_OP_BEGIN_ARGS,
    RPLNI_OP_END_ARGS,
    RPLNI_OP_BEGIN_LIST,
    RPLNI_OP_END_LIST,
    RPLNI_OP_IF,
    RPLNI_OP_ELSE,
    RPLNI_OP_ENDIF,
    RPLNI_OP_GOTO,
    RPLNI_OP_CALL,
    RPLNI_OP_RESTART,
    RPLNI_OP_LEN,
    RPLNI_OP_LIST_PUSH,
    RPLNI_OP_LIST_POP,
    RPLNI_OP_SPREAD,
    RPLNI_OP_SUM,
    RPLNI_OP_PUSH_CLOSURE,
    RPLNI_OP_INT,
    RPLNI_OP_STR,
    RPLNI_OP_PRINT,
    RPLNI_OP_INPUT,
    RPLNI_OP_EVAL,
    RPLNI_OP_OPTIMIZE,
    RPLNI_OP_COMPILE,
    RPLNI_OP_DEBUG,
    RPLNI_OP_SYSTEM,
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
    struct rplni_state* owner;
    enum rplni_func_type type;
    struct rplni_prog prog;
    /* strlist */
    struct rplni_ptrlist* params;
    /* strlist */
    struct rplni_ptrlist* members;
};

struct rplni_closure
{
    struct rplni_state* owner;
    struct rplni_func* funcdef;
    struct rplni_scope scope;
};


struct rplni_ptrlist
{
    size_t cap;
    size_t size;
    int is_strlist;
    int is_uniquelist;
    union {
        void** any;
        uintptr_t* _uint;
        struct rplni_str** str;
        struct rplni_list** list;
        struct rplni_func** func;
        struct rplni_closure** closure;
        char** cstr;
    } values;
};

struct rplni_loader {
    struct rplni_state* owner;
    struct rplni_ptrlist* names;
};


struct rplni_state
{
    struct rplni_scope scope;
    struct rplni_loader loader;
    struct rplni_list* data_stack;
    struct rplni_list* arg0_stack;
    struct rplni_list* callee_stack;
    struct rplni_list* list_head_stack;
    struct rplni_ptrlist* scope_stack;

    int calls_without_gc;
    int gc_interval;
    struct rplni_ptrlist* objs;
};

struct rplni_tmpstr
{
    size_t cap;
    size_t size;
    char* s;
};


int rplni_type_is_number(enum rplni_value_type type);
int rplni_type_is_callable(enum rplni_value_type type);
/* callable with length */
int rplni_type_is_arraylike(enum rplni_value_type type);

/* usage
* unref:  deallocates when --refs <= circular-refs
* del:  simple deallocation that ignores child nodes.
* limitation:
*   this structure will never be deallocated
*     A = [&B, &B]
*     B = [&A, &A]
*/

/* values */
/* values themselves are not managed by GC */

int rplni_value_init(struct rplni_value *value);
int rplni_value_init_with_uint(struct rplni_value* value, uintptr_t uint_value);
int rplni_value_init_with_sym(struct rplni_value* value, rplni_id_t id);
int rplni_value_init_with_cstr(struct rplni_value* value, size_t size, const char* str_value, struct rplni_state* state);
int rplni_value_init_with_captured_list(struct rplni_value* value, size_t size, struct rplni_list *src_stack, struct rplni_state *state);
int rplni_value_init_with_empty_func(struct rplni_value* value, enum rplni_func_type type, struct rplni_state* state);
int rplni_value_clean(struct rplni_value* value);  /* inir with 0 */
int rplni_value_find_living_objs(struct rplni_value* value, struct rplni_ptrlist* known_nodes);
int rplni_value_owner(struct rplni_value* value, struct rplni_state** out_owner);
int rplni_value_eq(const struct rplni_value* value, const struct rplni_value* value2, int unique);

/* does not touch refs */
int rplni_values_init(size_t size, struct rplni_value* values);
size_t rplni_values_object_index(size_t size, const struct rplni_value* values, const struct rplni_value* value);
int rplni_values_has_object(size_t size, const struct rplni_value* values, const struct rplni_value* value);
int rplni_values_add_object(
    size_t size, struct rplni_value* values,
    struct rplni_value* value,
    struct rplni_state* state,
    size_t* out_size, struct rplni_value* out_values);
int rplni_values_remove_object(size_t size, struct rplni_value* values, struct rplni_value* value);

int rplni_named_init(struct rplni_named *named, rplni_id_t id, struct rplni_state* state);
int rplni_named_clean(struct rplni_named *named, struct rplni_state* state);

/* when arg == NULL, operand becomes uint(0) */
int rplni_cmd_init(struct rplni_cmd* cmd, enum rplni_op_type op, struct rplni_value* arg, struct rplni_state* state);
int rplni_cmd_clean(struct rplni_cmd* cmd, struct rplni_state* state);
int rplni_cmd_del(struct rplni_cmd* cmd, struct rplni_state* state);
int rplni_cmd_count_circular_refs(struct rplni_cmd* cmd, void* root, struct rplni_ptrlist* known_nodes);
int rplni_prog_init(struct rplni_prog* prog, struct rplni_state* state);
int rplni_prog_add(struct rplni_prog* prog, struct rplni_cmd* cmd, struct rplni_state* state);
int rplni_prog_clean(struct rplni_prog* prog, struct rplni_state* state);
int rplni_prog_count_circular_refs(struct rplni_prog* prog, void* root, struct rplni_ptrlist* known_nodes);
int rplni_prog_run(struct rplni_prog* prog, struct rplni_state* state, size_t n_params);
int rplni_prog_find_endif(struct rplni_prog* prog, size_t idx, int ignore_els, size_t* out_index);
int rplni_prog_optimize(struct rplni_prog* prog, struct rplni_state* state);
int rplni_prog_compile(struct rplni_prog* prog,  const char *lang, struct rplni_state* state);

int rplni_scope_init(struct rplni_scope *scope, struct rplni_state* state);
int rplni_scope_clean(struct rplni_scope* scope);
int rplni_scope_find_living_objs(struct rplni_scope* scope, struct rplni_ptrlist* known_nodes);
int rplni_scope_add_var(struct rplni_scope* scope, rplni_id_t id);
size_t rplni_scope_var_index(const struct rplni_scope* scope, rplni_id_t id);
int rplni_scope_has_var(const struct rplni_scope* scope, rplni_id_t id);
int rplni_scope_load_var(struct rplni_scope* scope, rplni_id_t id, struct rplni_value* out_value);
int rplni_scope_store_var(struct rplni_scope* scope, rplni_id_t id, struct rplni_value* value);
int rplni_scope_load_var_by_index(struct rplni_scope* scope, size_t idx, struct rplni_value* out_value);
int rplni_scope_store_var_by_index(struct rplni_scope* scope, size_t idx, struct rplni_value* value);
int rplni_scope_store_var_by_name(struct rplni_scope* scope, const char* name, struct rplni_value* value);

/* root. interpreter state */
int rplni_state_init(struct rplni_state* state);
int rplni_state_clean(struct rplni_state* state);
int rplni_state_gc(struct rplni_state* state);
void* rplni_state_malloc(struct rplni_state* state, size_t size);
void* rplni_state_realloc(struct rplni_state* state, void* p, size_t size);
int rplni_state_free(struct rplni_state* state, void* p);
char* rplni_state_strdup(struct rplni_state* state, size_t size, const char* s);
int rplni_state_add(struct rplni_state* state, void* p);
int rplni_state_has(const struct rplni_state* state, const void* p);
int rplni_state_remove(struct rplni_state* state, void* p);
int rplni_state_current_scope(struct rplni_state* state, struct rplni_scope** out_scope);
int rplni_state_outer_scope(struct rplni_state* state, struct rplni_scope** out_scope);
int rplni_state_push_scope(struct rplni_state* state, struct rplni_scope* scope);
int rplni_state_pop_scope(struct rplni_state* state, struct rplni_scope** out_scope);
int rplni_state_add_var(struct rplni_state* state, rplni_id_t id);
int rplni_state_find_var(struct rplni_state* state, rplni_id_t id, struct rplni_scope** out_scope, size_t* out_index);
int rplni_state_load_var(struct rplni_state* state, rplni_id_t id, struct rplni_value* out_value);
int rplni_state_store_var(struct rplni_state* state, rplni_id_t id, struct rplni_value* value);
int rplni_state_has_scope(const struct rplni_state* state, const struct rplni_scope* scope);
int rplni_state_compare_scopes(
    const struct rplni_state* state,
    const struct rplni_scope* scope,
    const struct rplni_scope* scope2,
    int* out_result);
int rplni_state_add_name(struct rplni_state* state, char* name);
int rplni_state_has_name(const struct rplni_state* state, const char* name);
rplni_id_t rplni_state_id_by_name(const struct rplni_state* state, const char* name);
int rplni_state_name_by_id(const struct rplni_state* state, rplni_id_t id, const char** out_name);
int rplni_state_push_value(struct rplni_state* state, struct rplni_value* value);
int rplni_state_pop_value(struct rplni_state* state, struct rplni_value* out_value);
int rplni_state_add_argv(struct rplni_state* state, int argc, char *argv[]);
int rplni_state_add_builtin_func(struct rplni_state* state, char *name, enum rplni_op_type op, size_t argc);

/* objects */
/* objects will be deallocated by GC */

struct rplni_str *rplni_str_new(size_t size, const char *value, struct rplni_state* state);
int rplni_str_init(struct rplni_str *str, size_t size, const char *value, struct rplni_state* state);
int rplni_str_find_living_objs(struct rplni_str* str, struct rplni_ptrlist* known_nodes);
int rplni_str_add(struct rplni_str* str, const struct rplni_str* str2);
int rplni_str_add_cstr(struct rplni_str* str, size_t size, const char* str2);

struct rplni_list *rplni_list_new(size_t cap, struct rplni_state* state);
int rplni_list_init(struct rplni_list* list, size_t cap, struct rplni_state* state);
struct rplni_list* rplni_list_new_with_captured(size_t size, struct rplni_list* stack, struct rplni_state* statee);
int rplni_list_init_with_captured(struct rplni_list* list, size_t size, struct rplni_list* stack, struct rplni_state* state);
int rplni_list_find_living_objs(struct rplni_list* list, struct rplni_ptrlist* known_nodes);
int rplni_list_push(struct rplni_list *list, struct rplni_value *value);
int rplni_list_pop(struct rplni_list *list, struct rplni_value *out_value);
int rplni_list_to_cstr(struct rplni_list* list, char** out_cstr);

struct rplni_func* rplni_func_new(enum rplni_func_type type, struct rplni_state* state);
int rplni_func_init(struct rplni_func* func, enum rplni_func_type type, struct rplni_state* state);
int rplni_func_find_living_objs(struct rplni_func* func, struct rplni_ptrlist* known_nodes);
int rplni_func_add_param(struct rplni_func* func, rplni_id_t id);
int rplni_func_run(struct rplni_func* func, struct rplni_state* state);

struct rplni_closure* rplni_closure_new(struct rplni_func* funcdef, struct rplni_state *state);
int rplni_closure_init(struct rplni_closure* closure, struct rplni_func* funcdef, struct rplni_state* state);
int rplni_closure_find_living_objs(struct rplni_closure* closure, struct rplni_ptrlist* known_nodes);
int rplni_closure_run(struct rplni_closure* closure, struct rplni_state* state);

size_t rplni_arraylike_size(struct rplni_value* arraylike);
size_t rplni_arraylike_start(struct rplni_value* arraylike);
size_t rplni_arraylike_end(struct rplni_value* arraylike);

size_t rplni_callable_argc(struct rplni_value *callable);
int rplni_callable_run(struct rplni_value *callable, struct rplni_state *state);

/* array list for non-null unique pointers */
struct rplni_ptrlist *rplni_ptrlist_new(int as_uniquelist, struct rplni_state* state);
/* initialized as strlist that deallocates elements in del() */
struct rplni_ptrlist* rplni_ptrlist_new_as_strlist(struct rplni_state* state);
int rplni_ptrlist_init(struct rplni_ptrlist *nodes, int as_strlist, int as_uniquelist, struct rplni_state* state);
int rplni_ptrlist_del(struct rplni_ptrlist *nodes, struct rplni_state* state);
size_t rplni_ptrlist_index(const struct rplni_ptrlist* nodes, const void* node);
size_t rplni_ptrlist_uint_index(const struct rplni_ptrlist* nodes, uintptr_t node);
int rplni_ptrlist_has(const struct rplni_ptrlist *nodes, const void *node);
int rplni_ptrlist_has_uint(const struct rplni_ptrlist* list, uintptr_t value);
int rplni_ptrlist_add(struct rplni_ptrlist *nodes, void *node, struct rplni_state* state);
int rplni_ptrlist_add_uint(struct rplni_ptrlist* list, uintptr_t value, struct rplni_state* state);
int rplni_ptrlist_add_str(struct rplni_ptrlist *nodes, char *node, int copy_value, struct rplni_state* state);
int rplni_ptrlist_remove(struct rplni_ptrlist* nodes, void* node, struct rplni_state* state);
int rplni_ptrlist_push(struct rplni_ptrlist* nodes, void* node, struct rplni_state* state);
int rplni_ptrlist_push_uint(struct rplni_ptrlist* nodes, uintptr_t value, struct rplni_state* state);
int rplni_ptrlist_push_str(struct rplni_ptrlist* nodes, char* value, int copy_value, struct rplni_state* state);
int rplni_ptrlist_pop(struct rplni_ptrlist* nodes, void** out_node, struct rplni_state* state);
int rplni_ptrlist_clear(struct rplni_ptrlist *nodes, struct rplni_state* state);


struct rplni_tmpstr* rplni_tmpstr_new();
int rplni_tmpstr_init(struct rplni_tmpstr* tmpstr);
int rplni_tmpstr_del(struct rplni_tmpstr* tmpstr);
int rplni_tmpstr_add_cstr(struct rplni_tmpstr* tmpstr, size_t size, const char* cstr);
int rplni_tmpstr_add_uintptr(struct rplni_tmpstr* tmpstr, uintptr_t u);
int rplni_tmpstr_add_ptr(struct rplni_tmpstr* tmpstr, const void* p);
int rplni_tmpstr_add_value(struct rplni_tmpstr* tmp, const struct rplni_value* value, struct rplni_ptrlist* known_nodes);
int rplni_tmpstr_add_str(struct rplni_tmpstr* tmp, const struct rplni_str* str);
int rplni_tmpstr_add_list(struct rplni_tmpstr* tmp, struct rplni_list* list, struct rplni_ptrlist* known_nodes);
int rplni_tmpstr_add_func(struct rplni_tmpstr* tmp, const struct rplni_func* func);
int rplni_tmpstr_add_closure(struct rplni_tmpstr* tmp, const struct rplni_closure* closure);

int rplni_value_to_tmpstr(const struct rplni_value* value, struct rplni_tmpstr** out_tmpstr);
int rplni_str_to_tmpstr(const struct rplni_str* str, struct rplni_tmpstr** out_tmpstr);
int rplni_list_to_tmpstr(struct rplni_list* list, struct rplni_tmpstr** out_tmpstr);
int rplni_func_to_tmpstr(const struct rplni_func* func, struct rplni_tmpstr** out_tmpstr);


#ifndef NDEBUG
void rplni_dump_leaked(void);
#endif


#endif
