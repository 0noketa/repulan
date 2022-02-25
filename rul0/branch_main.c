#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


extern void repulan0_main();
extern void repulan0_push(uintptr_t);
extern uintptr_t repulan0_pop();
extern uintptr_t *repulan0_sp();
extern uintptr_t repulan0_call_func(uintptr_t, uintptr_t);
extern void repulan0_call_proc(uintptr_t, uintptr_t);

uintptr_t use_prompt = 1;
uintptr_t no_prompt = 0;
uintptr_t input_int(uintptr_t mode) {
    if (mode == use_prompt) fputs("input>", stderr);

    char buf[4] = {0,};
    uintptr_t x;
    fgets(buf, 3, stdin);

    return atol(buf);
}
uintptr_t print_int(uintptr_t x) {
    printf("%u\n", (int)x);

    return x;
}


int main() {
    repulan0_main();

    return 0;
}
