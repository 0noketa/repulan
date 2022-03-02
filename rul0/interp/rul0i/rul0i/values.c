#include "rul0i.h"


int rul0i_value_init(struct rul0i_value *value)
{
    if (value == NULL) return 0;

    value->type = RUL0I_UINT;
    value->value._uint = 0;

    return 1;
}
int rul0i_value_clean(struct rul0i_value *value, struct rul0i_gc_nodes *nodes)
{
    if (value == NULL) return 0;

    switch (value->type)
    {
        case RUL0I_UINT:
            return 1;
        case RUL0I_STR:
            if (!rul0i_str_unref(value->value._str, nodes)) return 0;
            break;
        case RUL0I_LIST:
            if (!rul0i_list_unref(value->value._list, nodes)) return 0;
            break;
        default:
            return 0;
    }

    value->type = RUL0I_UINT;
    value->value._uint = 0;

    return 1;
}
int rul0i_value_ref(struct rul0i_value* value)
{
    if (value == NULL) return 0;

    switch (value->type)
    {
    case RUL0I_UINT:
        return 1;
    case RUL0I_STR:
        if (!rul0i_str_ref(value->value._str)) return 0;
        break;
    case RUL0I_LIST:
        if (!rul0i_list_ref(value->value._list)) return 0;
        break;
    default:
        return 0;
    }

    return 1;
}

int rul0i_named_init(struct rul0i_named *named, char *name)
{
    if (named == NULL || name == NULL) return 0;

    named->name = _strdup(name);
    rul0i_value_init(&(named->value));

    return named->name != NULL;
}
int rul0i_named_clean(struct rul0i_named *named, struct rul0i_gc_nodes *nodes)
{
    if (named == NULL) return 0;

    rul0i_value_clean(&(named->value), nodes);

    free(named->name);
    named->name = NULL;

    return 1;
}

int rul0i_scope_init(struct rul0i_scope *scope)
{
    if (scope == NULL) return 0;

    scope->size = 0;
    scope->vars = malloc(0);

    return scope->vars != NULL;
}
int rul0i_scope_clean(struct rul0i_scope *scope, struct rul0i_gc_nodes *nodes)
{
    if (scope == NULL) return 0;

    int owns_nodes = nodes == NULL;

    if (owns_nodes) nodes = rul0i_gc_nodes_new();

    for (size_t i = 0; i < scope->size; ++i)
    {
        rul0i_named_clean(scope->vars + i, nodes);
    }

    if (owns_nodes) rul0i_gc_nodes_clean(nodes);

    free(scope->vars);

    scope->size = 0;
    scope->vars = NULL;

    return 1;
}
int rul0i_scope_add(struct rul0i_scope *scope, struct rul0i_value *value)
{
    return 0;
}

int rul0i_state_init(struct rul0i_state *state)
{
    if (state == NULL) return 0;

    state->data_stack = NULL;
    state->arg0_stack = NULL;
    state->temp_stack = NULL;

    if (!rul0i_scope_init(&(state->global_scope))) return 0;

    state->data_stack = rul0i_list_new(RUL0I_DATA_STACK_DEFAULT_CAP);
    state->arg0_stack = rul0i_list_new(RUL0I_ARG0_STACK_DEFAULT_CAP);
    state->temp_stack = rul0i_list_new(RUL0I_TEMP_STACK_DEFAULT_CAP);

    if (state->data_stack && state->arg0_stack && state->temp_stack) return 1;

    rul0i_list_unref(state->data_stack, NULL);
    rul0i_list_unref(state->arg0_stack, NULL);
    rul0i_list_unref(state->temp_stack, NULL);

    state->data_stack = NULL;
    state->arg0_stack = NULL;
    state->temp_stack = NULL;

    return 0;
}
