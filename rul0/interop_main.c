#include <stdio.h>
#include <stdint.h>


extern void repulan0_main();
extern void repulan0_push(uintptr_t);
extern uintptr_t repulan0_pop();
extern uintptr_t *repulan0_sp();
extern uintptr_t repulan0_call_func(uintptr_t, uintptr_t);
extern void repulan0_call_proc(uintptr_t, uintptr_t);

uintptr_t print_int(uintptr_t x) {
    printf("%u", (int)x);

    return x;
}
uintptr_t print_char(uintptr_t x) {
    putchar((int)x);

    return x;
}
uintptr_t println_int(uintptr_t x) {
    printf("%u\n", (int)x);

    return x;
}

uintptr_t loop(uintptr_t x) {
    uintptr_t f = repulan0_pop();

    for (uintptr_t i = 0; i < x; ++i)
        repulan0_call_proc(f, i);

    return f;
}
uintptr_t loop2(uintptr_t x) {
    uintptr_t f = repulan0_pop();
    uintptr_t g = repulan0_pop();

    repulan0_call_proc(g, x);

    for (uintptr_t i = 0; i < x; ++i)
        repulan0_call_proc(f, i);

    repulan0_push(g);

    return f;
}


int main() {
    repulan0_main();

    return 0;
}
