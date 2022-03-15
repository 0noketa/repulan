#ifndef rplni_loader__h
#define rplni_loader__h

#include "rplni.h"


typedef uint16_t rplni_id_t;
#define RPLNI_ID_MAX UINT16_MAX


struct rplni_loader {
    struct rplni_ptrlist* names;
};


int rplni_loader_init(struct rplni_loader* loader);
int rplni_loader_clean(struct rplni_loader* loader);
int rplni_loader_add_name(struct rplni_loader* loader, char* name, int copy_str);
rplni_id_t rplni_loader_name_index(struct rplni_loader* loader, char* name);
int rplni_loader_has_name(struct rplni_loader* loader, char* name);
int rplni_loader_name_by_index(struct rplni_loader* loader, rplni_id_t id, char** out_name);
/* if out_tkn is not NULL, callar should not deallocate result stored on it */
int rplni_loader_read(struct rplni_loader* loader,
    size_t size, char* src,
    size_t* out_start, size_t* out_end, char** out_tkn, rplni_id_t* out_id,
    size_t* out_next);


int rplni_prog_load(
    struct rplni_prog* prog,
    size_t size, char* src,
    struct rplni_ptrlist* params, struct rplni_scope* scope);



#endif
