#include <stdio.h>
#include <ctype.h>
#include "rplni.h"
#include "rplni_loader.h"
#include "rplni_macro.h"


int main(int argc, char *argv[])
{
    struct rplni_loader loader;
    if (!rplni_loader_init(&loader))
    {
        puts("failed to init loader");
        return 1;
    }

    char* src = "\\\\argx{argx==12?3|4?!=argx argx argx\"hello\"}(1 5:)";
    size_t end = strlen(src);

    for (size_t i = 0; i < end;)
    {
        size_t tkn_start;
        size_t tkn_end;
        char* tkn;
        rplni_id_t id;
        if (rplni_loader_read(&loader, end, src + i, &tkn_start, &tkn_end, &tkn, &id))
        {
            printf("%d: [%s](%d-%d)\n", (int)id, tkn, (int)(i + tkn_start), (int)(i + tkn_end));

            i += tkn_end;
        }
        else
        {
            break;
        }
    }

    rplni_loader_clean(&loader);

#ifndef NDEBUG
    rplni_dump_leaked();
#endif

    return 0;
}
