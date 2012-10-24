// Alejo Sanchez 2012
// Generate 1m int32 in the range 0:99999999 and write to stdout
#include <stdint.h>
#include <stdio.h>
#include <time.h>

int main() {
    int32_t   i, v;

    srand(time(NULL));
    for (i = 0; i < 1000000; i++) {
        v = rand() % 99999999;
        fwrite( &v, sizeof (v), 1, stdout);
    }

    return 0;
}
