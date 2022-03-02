#include "rul0i.h"


struct rul0i_gc_nodes *rul0i_gc_nodes_new()
{
    struct rul0i_gc_nodes *nodes = malloc(sizeof(struct rul0i_gc_nodes));

    if (nodes != NULL)
    {
        if (rul0i_gc_nodes_init(nodes)) return nodes;

        free(nodes);
    }

    return NULL;
}
int rul0i_gc_nodes_init(struct rul0i_gc_nodes *nodes)
{
    if (nodes == NULL) return 0;

    nodes->size = 0;
    nodes->nodes = malloc(RUL0I_LIST_BLOCK_SIZE * sizeof(void*));

    return nodes->nodes != NULL;
}
int rul0i_gc_nodes_clean(struct rul0i_gc_nodes *nodes)
{
    if (nodes == NULL) return 0;
    if (nodes->nodes == NULL) return 1;

    free(nodes->nodes);
    nodes->nodes = NULL;

    return 1;
}
int rul0i_gc_nodes_has(struct rul0i_gc_nodes *nodes, void *node)
{
    if (nodes == NULL) return 0;

    for (size_t i = 0; i < nodes->size; ++i)
    {
        if (nodes->nodes[i] == node) return 1;
    }

    return 0;
}
int rul0i_gc_nodes_add(struct rul0i_gc_nodes *nodes, void *node)
{
    if (nodes == NULL) return 0;
    if (rul0i_gc_nodes_has(nodes, node)) return 1;

    void **a = realloc(nodes->nodes, RUL0I_LIST_CEILED_SIZE(nodes->size + 1) * sizeof(void*));

    if (a == NULL) return 0;

    nodes->nodes = a;
    nodes->nodes[nodes->size++] = node;

    return 1;
}
