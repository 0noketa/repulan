#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


extern void repulan0_main();
extern void repulan0_push(uintptr_t);
extern uintptr_t repulan0_pop();
extern uintptr_t *repulan0_sp();
extern uintptr_t repulan0_get(uintptr_t, uintptr_t);
extern uintptr_t repulan0_len(uintptr_t);
extern uintptr_t repulan0_call_func(uintptr_t, uintptr_t);
extern void repulan0_call_proc(uintptr_t, uintptr_t);

uintptr_t check(uintptr_t label) {
    char c = ' ';
    uintptr_t *sp = repulan0_sp();

    printf("label %08d @ %p:", (int)label, sp);
    for (uintptr_t *p = sp + 8; p > sp - 8; --p) {
        if (p == sp) c = '*';
        if (p == sp - 1) c = '(';
        if (p == sp - 2) c = ' ';
        printf("%c%6d", c, (int)*p);
    }

    puts("");

    return 1;
}

uintptr_t rul0c_readline(uintptr_t max_chars) {
    uint8_t buf[256] = {0,};

            // repulan0_push('A');

    if (feof(stdin)) return 0;

    if (fgets(buf, (max_chars < 254 ? max_chars : 254), stdin)) {
        uint8_t *p = buf;
        for (; *p && *p != '\n' && *p != '\r' && *p != 26; ++p) {
            repulan0_push(*p);
        }
    }

    return !feof(stdin);
}
uintptr_t rul0c_writeline(uintptr_t line) {
    uintptr_t len = repulan0_len(line);
    printf("[len:%d]\n", (int)len);

    for (uintptr_t i = 0; i < len; ++i) {
        printf("%d ", (int)repulan0_get(line, i));
    }

    puts("");

    return 1;
}
uintptr_t rul0c_writeline_str(uintptr_t line) {
    uintptr_t len = repulan0_len(line);
    printf("[len:%d]\n", (int)len);

    for (uintptr_t i = 0; i < len; ++i) {
        printf("%c ", (int)repulan0_get(line, i));
    }

    puts("");

    return 1;
}


int main() {
    repulan0_main();

    return 0;
}
