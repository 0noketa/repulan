#include <stdio.h>
#include <stdint.h>


extern void repulan0_main();
extern void repulan0_push(uintptr_t);
extern uintptr_t repulan0_pop();
extern uintptr_t *repulan0_sp();
// extern uintptr_t repulan0_array_buf();
// extern uintptr_t new_array_idx;
// extern uintptr_t new_array_size;
// extern uintptr_t new_array_offset;

uintptr_t cfunc(uintptr_t x) {
    // printf("array{idx: %u, size:%u, offset:%08u}\n", (int)new_array_idx, (int)new_array_size, (int)new_array_offset);

    return x;
}

uintptr_t *capture_start;
uintptr_t begin_capture(uintptr_t x) {
    capture_start = repulan0_sp();

    return 0;
}
uintptr_t end_capture(uintptr_t x) {
    uintptr_t *capture_end = repulan0_sp();

    printf("%u - %u (%u)\n", (int)capture_start, (int)capture_end, (int)(capture_start - capture_end));

    for (uintptr_t *p = capture_start; p-- > capture_end;) {
        printf("%u ", (int)*p);
    }

    puts(": captured");


    // p = (void*)repulan0_array_buf();
    // printf("%u - %u (array[%u] of %u):", (int)p, (int)&p[0x40*0x20], (int)0x20, (int)0x40);
    // for (int i = 0; i < 0x40*0x20/4; ++i) {
    //     if (i % 10 == 0) printf("\n%06u:", i);
    //     printf("%u ", (int)p[i]);
    // }
    // puts(": array");

    return 0;
}


int main() {
    repulan0_main();

    return 0;
}
