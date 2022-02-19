#include <stdio.h>
#include <stdint.h>


extern void repulan0_main();

uintptr_t cfunc(uintptr_t x) {
    printf("%06d (%08X)\n", (int)x, (int)x);

    return x << 8;
}

int main() {
    repulan0_main();

    return 0;
}
