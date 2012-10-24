// Copyright (C) 2011-2012 Alejo Sanchez  (alejo@ologan.com)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <sys/time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>


// Print to standard error and exit
#define error_exit(format, ...) \
    do { \
        fprintf(stderr, format, ##__VA_ARGS__); \
        exit(-1); \
    } while(0)

// Prediction in fixed point prediction = predict[i]/
uint32_t predict[256];

typedef struct {
    int32_t prediction;
    int16_t count;
} Model;
Model m[256][256]; // Context order 2 and bit context
 
static int compress(const uint8_t *in, size_t len, FILE *out);
static int encode(uint8_t bit, uint32_t pred, uint32_t *low, uint32_t *high,
        FILE *out);
static void update_model(uint8_t bit, Model *m);
static int decompress(const uint8_t *in, size_t len, FILE *out);
static int decode(uint32_t pred, uint32_t *low, uint32_t *high,
        uint32_t *curr);

int
main(int argc, char *argv[]) {
    struct     timeval cstart, cend; // Time markers for benchmarking
    struct     stat fs;
    FILE      *out;
    uint8_t   *d;                    // Data buffer
    uint32_t   cusec;                // Counters for stats
    int        fd;
    int        tot;                  // Total decompressed
    uint32_t   size = 1<<21;
    int32_t    remaining;
    float      mbps;

    if (argc < 3)
        error_exit("usage: %s <input file> <output file>\n", argv[0]);

    if ((fd = open(argv[1], O_RDONLY)) < 0)
        error_exit("error opening input file %s\n", argv[1]);

    fstat(fd, &fs);                  // Get file stats

    // Output file
    if ((out = fopen(argv[2], "w")) == NULL)
        error_exit("error opening output file %s\n", argv[1]);

    // mmap the input file, no caching since it's one time read
    d = mmap(NULL, size, PROT_READ, MAP_NOCACHE | MAP_PRIVATE, fd, 0);

    if (d == MAP_FAILED)
        error_exit("%s\n", strerror(errno));

    //
    // Compression
    //
    gettimeofday(&cstart, NULL);     // Start timing compression

    // Compress the file
    remaining = compress(d, size, out);  // Process

    gettimeofday(&cend, NULL);       // End timing compression

    cusec = ((cend.tv_sec - cstart.tv_sec) * 1000000L)
            + (cend.tv_usec - cstart.tv_usec);

    mbps = (float) (1000000L * size>>20) / cusec;
    printf("%9d bytes   compressed in %9d usec, %.2f MB/s remaining %d%%\n",
            size, cusec, mbps, remaining * 100 / size);

    munmap(d, size);
    close(fd);
    fclose(out);

    if ((fd = open(argv[2], O_RDONLY)) < 0)
        error_exit("error opening file <%s> for decompression\n", argv[2]);

    fstat(fd, &fs);                  // Get file size
    size = fs.st_size;

    // mmap the compressed file, no caching since its one time read
    d = mmap(NULL, fs.st_size, PROT_READ, MAP_NOCACHE | MAP_PRIVATE, fd, 0);

    //
    // Decompression (test)
    //
    gettimeofday(&cstart, NULL);

    // Compress the file
    tot = decompress(d, size, out);  // Process

    gettimeofday(&cend, NULL);       // End timing compression

    cusec = ((cend.tv_sec - cstart.tv_sec) * 1000000L)
            + (cend.tv_usec - cstart.tv_usec);

    mbps = (float) (1000000L * size>>20) / cusec;
    printf("%9d bytes decompressed in %9d usec, %.2f MB/s\n",
            tot, cusec, mbps);

    return 0;
}

static void
init_models(void) {
    for (int i = 0; i < 256; i++) {
        for (int j = 0; j < 256; j++) {
            m[i][j].prediction = 65536/2 - 1;
            m[i][j].count      = 0;
        }
    }
}

static int
compress(const uint8_t *in, size_t len, FILE *out) {
    // Initial state
    uint32_t  low = 1;
    uint32_t high = 0xffffffff;
    int         w = 0;          // Bytes written

    init_models();

    // Start encoding 2nd byte (first byte has no previous)
    for (int i = 0, a = 0, b, x = 0; i < (len - 1); i++) {

        b = a;
        a = x;
        x = in[i];

        for (int j = 7 ; j >= 0; --j) {

            int ctx = (x + 256) >> (j + 1); // bit context
            int bit = (x >> j) % 2;

            Model *mp = &m[a][ctx];
            w         += encode(bit, mp->prediction, &low, &high, out);

            update_model(bit, mp);

        }

    }

    putc(high >> 24, out);
    putc(high >> 16, out);
    putc(high >>  8, out);
    putc(high, out);
    putc(0, out);
    putc(0, out);
    putc(0, out);
    putc(0, out);

    return w;
}

// Encode bit y with probability p/65536
static int
encode(uint8_t bit, uint32_t pred, uint32_t *low, uint32_t *high,
        FILE *out)
{
    int w = 0;

    uint32_t mid = *low + ((uint64_t) (*high - *low) * pred >> 16);

    if (bit)
        *high = mid;    // bit 1 pick high 
    else
        *low = mid + 1; // bit 0 pick low, skip 0

    // Write identical leading bytes
    while ((*high ^ *low) < 0x1000000) { // write identical leading bytes

        putc(*high >> 24, out);
        w++;

        *high  =  *high << 8 | 255;
        *low   =  *low << 8;
        *low  += (*low == 0); // so we don't code 4 0 bytes in a row
    }

    return w;

}

static void
update_model(uint8_t bit, Model *m) {
    const int DELTA = 32767;
    const int LIMIT = 10;

    if (m->count < LIMIT)
        ++m->count;

    int64_t error = ((((int64_t) bit << 16) - m->prediction)<<16);
    m->prediction += error / ((m->count << 16) + DELTA);

}

//
// Decoder
//
  
static int
decompress(const uint8_t *in, size_t len, FILE *out) {
    // Initial state
    uint32_t   low = 1;
    uint32_t  high = 0xffffffff;
    uint32_t  curr = 0;
    int        tin;              // Total bytes read from input

    init_models();

    for (tin = 0; tin < 4; tin++)
        curr = curr << 8 | in[tin];  // first 4 bytes of input

    for (int a = 0, b = 0; tin < (len - 1); )
    {
        int  ctx;  // bit context and output byte

        for (ctx = 1; ctx < 256; )
        {
            int bit;

            Model *mp = &m[a][ctx];
            if ((bit = decode(mp->prediction, &low, & high, &curr)) == -1)
                error_exit("archive corrupted at %d of %ld\nhi %u\nlo %u\n",
                        tin, len, high, low);

            update_model((uint8_t) bit, mp);

            // Shift out identical leading bytes, read more input
            while ((high ^ low) < 0x1000000) {

                high  = (high << 8) | 255;
                low   = (low  << 8);
                low  += (low == 0); // Avoid all zeroes

                if (tin >= len)
                    error_exit("unexpected end of file");

                curr = curr << 8 | in[tin++];

            }

            ctx = ctx * 2 + bit;

        }

        ctx -= 256;        // Decoded byte
        // putc(ctx, stdout);
        b = a;
        a = ctx;

    }

    return tin + 1; // count starts at 1

}

// Return decoded bit from file 'in' with probability p/65536
static int
decode(uint32_t pred, uint32_t *low, uint32_t *high, uint32_t *curr) {

    uint32_t mid = *low + ((uint64_t)(*high - *low) * pred >> 16);

    if (*curr < *low || *curr > *high)
        return (-1);

    uint8_t bit = (*curr <= mid); // What half is current in

    // Adjust to new range (half)
    if (bit)
        *high = mid;
    else
        *low  = mid + 1;

    return bit;

}
