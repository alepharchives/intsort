#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int32_t ints[1000000]; // random 27bit integers

int cmpi32(const void *a, const void *b) {
    return ( *(int32_t *)a - *(int32_t *)b );
}

int main() {
    int32_t *pi = ints; // Pointer to input ints (REPLACE W/ read from net)

    // Fill pseudo random integers of 27 bits (not seeded)
    srand(time(NULL));
    for (int i = 0; i < 1000000; i++)
        ints[i] = rand() % 99999999; // random 32bit in range 0:99999999

    qsort(ints, 1000000, sizeof (ints[0]), cmpi32); // Sort 1000000 int32s

    // Now delta encode, optional, store differences to previous int
    for (int i = 1, prev = ints[0]; i < 1000000; i++) {
        ints[i] -= prev;
        prev    += ints[i];
    }

    FILE *f = fopen("ints.bin", "w");
    fwrite(ints, 4, 1000000, f);
    fclose(f);
    exit(0);

}
