#include "example-shared.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void
quit(int e, ...) {
    /* frees a bunch of stuff and exits */
    va_list ap;
    void *p = NULL;

    va_start(ap,e);
    do {
        p = va_arg(ap,void *);
        if(p != NULL) {
            free(p);
        }
    } while(p != NULL);

    EXAMPLE_EXIT(e);
}

void
repack_samples_deinterleave(int32_t **d, int16_t *s, uint32_t channels, uint32_t num, uint8_t scale) {
    uint32_t i = 0;
    uint32_t c = 0;
    while(c < channels) {
        i = 0;
        while(i < num) {
            d[c][i] = (int16_t)(((uint16_t)s[(i*channels)+c]) >> scale);
            i++;
        }
        c++;
    }
}

void
pack_uint32le(uint8_t *d, uint32_t n) {
    d[0] = (uint8_t)(n       );
    d[1] = (uint8_t)(n >> 8  );
    d[2] = (uint8_t)(n >> 16 );
    d[3] = (uint8_t)(n >> 24 );
}

size_t write_uint16(FILE *f, uint16_t u) {
    uint8_t buffer[2];
    buffer[0] = (uint8_t)(u >> 8 );
    buffer[1] = (uint8_t)(u      );
    return fwrite(buffer,1,2,f);
}

size_t write_uint32(FILE *f, uint32_t u) {
    uint8_t buffer[4];
    buffer[0] = (uint8_t)(u >> 24);
    buffer[1] = (uint8_t)(u >> 16);
    buffer[2] = (uint8_t)(u >> 8 );
    buffer[3] = (uint8_t)(u      );
    return fwrite(buffer,1,4,f);
}

size_t write_uint64(FILE *f, uint64_t u) {
    uint8_t buffer[8];
    buffer[0] = (uint8_t)(u >> 56);
    buffer[1] = (uint8_t)(u >> 48);
    buffer[2] = (uint8_t)(u >> 40);
    buffer[3] = (uint8_t)(u >> 32);
    buffer[4] = (uint8_t)(u >> 24);
    buffer[5] = (uint8_t)(u >> 16);
    buffer[6] = (uint8_t)(u >> 8 );
    buffer[7] = (uint8_t)(u      );
    return fwrite(buffer,1,8,f);
}

size_t write_double(FILE *f, double v) {
    union {
        double f;
        uint64_t d;
    } tmp;

    tmp.f = v;
    return write_uint64(f,tmp.d);
}

