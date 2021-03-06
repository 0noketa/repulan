#ifndef rplni_loader__h
#define rplni_loader__h

#include "rplni.h"



int rplni_loader_init(struct rplni_loader* loader);
int rplni_loader_clean(struct rplni_loader* loader);
int rplni_loader_add_name(struct rplni_loader* loader, char* name, int copy_str);
rplni_id_t rplni_loader_name_index(const struct rplni_loader* loader, const char* name);
int rplni_loader_has_name(const struct rplni_loader* loader, const char* name);
int rplni_loader_name_by_index(const struct rplni_loader* loader, rplni_id_t id, const char** out_name);
/* if out_tkn is not NULL, callar should not deallocate result stored on it */
int rplni_loader_read(struct rplni_loader* loader,
    size_t size, const char* src,
    size_t* out_start, size_t* out_end, const char** out_tkn, rplni_id_t* out_id);


int rplni_prog_load(
    struct rplni_prog* prog,
    size_t size, const char* src,
    const struct rplni_ptrlist* params,
    struct rplni_ptrlist* members,
    struct rplni_state* state,
    size_t* out_next);

int rplni_func_load(
    struct rplni_func* func,
    size_t size, const char* src,
    struct rplni_state* state,
    size_t* out_next);

int rplni_state_eval(
    struct rplni_state* state,
    size_t size, const char* src);



#endif
