/*
Copyright (c) 2022 John Regan

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef TECHNICALLYALAC_H
#define TECHNICALLYALAC_H

#include <stdint.h>
#include <stddef.h>
#include <assert.h>

typedef struct technicallyalac_s technicallyalac;

#if defined(__GNUC__) && __GNUC__ >= 2 && __GNUC_MINOR__ >= 5
#define TF_PURE __attribute__((const))
#endif

#ifndef TF_PURE
#define TF_PURE
#endif

/* returns the size of a technicallyalac object */
TF_PURE
size_t technicallyalac_size(void);

/* returns the number of bytes required for the "magic cookie" */
/* other codecs call this (x)Specific Config, like AAC Specific Config */
/* libavformat/ffmpeg call this "extradata" */
TF_PURE
uint32_t technicallyalac_size_cookie(void);

/* initialize a technicallyalac object, should be called before any other function */
/* channels should be number of channels (1-2) */
int technicallyalac_init(technicallyalac *f, uint32_t framelength, uint32_t samplerate, uint8_t channels, uint8_t bitdepth);

/* get the regular/average packet size, in bytes */
uint32_t technicallyalac_packet_size(technicallyalac *f);

/* get the maximum packet size, in bytes */
uint32_t technicallyalac_max_packet_size(technicallyalac *f);

/* writes out the cookie */
int technicallyalac_cookie(technicallyalac *f, uint8_t *output, uint32_t *bytes);

/* write out a packet of audio. num_frames should be equal to your pre-configured framelength, except for the last alac frame (where it may be less). */
int technicallyalac_packet(technicallyalac *f, uint8_t *output, uint32_t *bytes, uint32_t num_frames, int32_t **frames);

enum TECHNICALLYALAC_COOKIE_STATE {
    TECHNICALLYALAC_COOKIE_START,
    TECHNICALLYALAC_COOKIE_FRAME_LENGTH,
    TECHNICALLYALAC_COOKIE_COMPATIBLE_VERSION,
    TECHNICALLYALAC_COOKIE_BIT_DEPTH,
    TECHNICALLYALAC_COOKIE_TUNING_PB,
    TECHNICALLYALAC_COOKIE_TUNING_MB,
    TECHNICALLYALAC_COOKIE_TUNING_KB,
    TECHNICALLYALAC_COOKIE_CHANNELS,
    TECHNICALLYALAC_COOKIE_MAXRUN,
    TECHNICALLYALAC_COOKIE_MAXFRAMEBYTES,
    TECHNICALLYALAC_COOKIE_AVGBITRATE,
    TECHNICALLYALAC_COOKIE_SAMPLERATE,
    TECHNICALLYALAC_COOKIE_END,
};

enum TECHNICALLYALAC_PACKET_STATE {
    TECHNICALLYALAC_PACKET_START,
    TECHNICALLYALAC_PACKET_CHANNEL,
    TECHNICALLYALAC_PACKET_END,
    TECHNICALLYALAC_PACKET_FLUSH,
};

enum TECHNICALLYALAC_CHANNEL_STATE {
    TECHNICALLYALAC_CHANNEL_START,
    TECHNICALLYALAC_CHANNEL_CHANMAP,
    TECHNICALLYALAC_CHANNEL_TAG,
    TECHNICALLYALAC_CHANNEL_HEADER_BITS,
    TECHNICALLYALAC_CHANNEL_SAMPLE_COUNT,
    TECHNICALLYALAC_CHANNEL_EXTRA_BITS,
    TECHNICALLYALAC_CHANNEL_ESCAPE,
    TECHNICALLYALAC_CHANNEL_SIZE,
    TECHNICALLYALAC_CHANNEL_DATA,
};

struct technicallyalac_cookie_state {
    enum TECHNICALLYALAC_COOKIE_STATE state;
};

struct technicallyalac_channel_state {
    enum TECHNICALLYALAC_CHANNEL_STATE state;
    uint32_t frame;
};

struct technicallyalac_packet_state {
    enum TECHNICALLYALAC_PACKET_STATE state;
    uint8_t channel;
};

struct technicallyalac_bitwriter_s {
    uint64_t val;
    uint8_t  bits;
    uint32_t pos;
    uint32_t len;
    uint8_t* buffer;
};

struct technicallyalac_s {
    uint32_t framelength;
    uint32_t samplerate;
    uint8_t channels;
    uint8_t bitdepth;
    uint8_t samplesize;

    struct technicallyalac_bitwriter_s bw;
    struct technicallyalac_cookie_state   si_state;
    struct technicallyalac_channel_state ch_state;
    struct technicallyalac_packet_state pa_state;
};

#ifdef __cplusplus
}
#endif

#endif

#ifdef TECHNICALLYALAC_IMPLEMENTATION
#define TECHNICALLYALAC_COOKIE_SIZE 24

typedef struct technicallyalac_bitwriter_s technicallyalac_bitwriter;

static void technicallyalac_bitwriter_init(technicallyalac_bitwriter *bw);
static int technicallyalac_bitwriter_add(technicallyalac_bitwriter *bw, uint8_t bits, uint64_t val);
static void technicallyalac_bitwriter_align(technicallyalac_bitwriter *bw);

static void technicallyalac_bitwriter_init(technicallyalac_bitwriter *bw) {
    bw->val    = 0;
    bw->bits   = 0;
}

/* flush out any bits that we can */
static void technicallyalac_bitwriter_flush(technicallyalac_bitwriter *bw) {
    uint64_t avail = ((bw->len - bw->pos) * 8);
    uint64_t mask  = -1LL;
    uint8_t  byte  = 0;

    while(avail && bw->bits > 7) {
        bw->bits -= 8;
        byte = (uint8_t)((bw->val >> bw->bits) & 0xFF);
        bw->buffer[bw->pos++] = byte;
        avail -= 8;
    }
    if(bw->bits == 0) {
        bw->val = 0;
    } else {
        mask >>= 64 - bw->bits;
        bw->val &= mask;
    }
}

static int technicallyalac_bitwriter_add(technicallyalac_bitwriter *bw, uint8_t bits, uint64_t val) {
    uint64_t mask  = -1LL;
    if(bw->bits + bits > 64) {
        return 0;
    }
    mask >>= (64 - bits);
    bw->val <<= bits;
    bw->val |= val & mask;
    bw->bits += bits;
    return 1;
}

static void technicallyalac_bitwriter_align(technicallyalac_bitwriter *bw) {
    uint8_t r = bw->bits % 8;
    if(r) {
        technicallyalac_bitwriter_add(bw,8-r,0);
    }
}

size_t technicallyalac_size(void) {
    return sizeof(technicallyalac);
}

uint32_t technicallyalac_size_cookie(void) {
    return TECHNICALLYALAC_COOKIE_SIZE;
}

int technicallyalac_init(technicallyalac *f, uint32_t framelength, uint32_t samplerate, uint8_t channels, uint8_t bitdepth) {

    f->framelength  = framelength;
    f->samplerate = samplerate;
    f->channels   = channels;
    f->bitdepth   = bitdepth;

    if(f->bitdepth < 4 || f->bitdepth > 32) {
        return -1;
    }

    if(f->channels < 1 || f->channels > 2) return -1;

    f->si_state.state   = TECHNICALLYALAC_COOKIE_START;
    f->ch_state.state   = TECHNICALLYALAC_CHANNEL_START;
    f->pa_state.state   = TECHNICALLYALAC_PACKET_START;

    f->ch_state.frame = 0;
    f->pa_state.channel = 0;

    technicallyalac_bitwriter_init(&f->bw);

    return 0;
}

int technicallyalac_cookie(technicallyalac *f,uint8_t *output, uint32_t *bytes) {
    int r = 1;

    if(output == NULL || bytes == NULL || *bytes == 0) {
        return TECHNICALLYALAC_COOKIE_SIZE;
    }

    f->bw.buffer = output;
    f->bw.len = *bytes;
    f->bw.pos = 0;

    while(f->bw.pos < f->bw.len && r) {
        technicallyalac_bitwriter_flush(&f->bw);

        switch(f->si_state.state) {
            case TECHNICALLYALAC_COOKIE_START: {
                technicallyalac_bitwriter_init(&f->bw);
                f->si_state.state = TECHNICALLYALAC_COOKIE_FRAME_LENGTH;
                break;
            }
            case TECHNICALLYALAC_COOKIE_FRAME_LENGTH: {
                if(technicallyalac_bitwriter_add(&f->bw,32,f->framelength)) {
                    f->si_state.state = TECHNICALLYALAC_COOKIE_COMPATIBLE_VERSION;
                }
                break;
            }
            case TECHNICALLYALAC_COOKIE_COMPATIBLE_VERSION: {
                if(technicallyalac_bitwriter_add(&f->bw,8,0)) {
                    f->si_state.state = TECHNICALLYALAC_COOKIE_BIT_DEPTH;
                }
                break;
            }
            case TECHNICALLYALAC_COOKIE_BIT_DEPTH: {
                if(technicallyalac_bitwriter_add(&f->bw,8,f->bitdepth)) {
                    f->si_state.state = TECHNICALLYALAC_COOKIE_TUNING_PB;
                }
                break;
            }
            case TECHNICALLYALAC_COOKIE_TUNING_PB: {
                if(technicallyalac_bitwriter_add(&f->bw,8,40)) {
                    f->si_state.state = TECHNICALLYALAC_COOKIE_TUNING_MB;
                }
                break;
            }
            case TECHNICALLYALAC_COOKIE_TUNING_MB: {
                if(technicallyalac_bitwriter_add(&f->bw,8, 10)) {
                    f->si_state.state = TECHNICALLYALAC_COOKIE_TUNING_KB;
                }
                break;
            }
            case TECHNICALLYALAC_COOKIE_TUNING_KB: {
                if(technicallyalac_bitwriter_add(&f->bw,8,14)) {
                    f->si_state.state = TECHNICALLYALAC_COOKIE_CHANNELS;
                }
                break;
            }
            case TECHNICALLYALAC_COOKIE_CHANNELS: {
                if(technicallyalac_bitwriter_add(&f->bw,8,f->channels)) {
                    f->si_state.state = TECHNICALLYALAC_COOKIE_MAXRUN;
                }
                break;
            }
            case TECHNICALLYALAC_COOKIE_MAXRUN: {
                if(technicallyalac_bitwriter_add(&f->bw,16,255)) {
                    f->si_state.state = TECHNICALLYALAC_COOKIE_MAXFRAMEBYTES;
                }
                break;
            }
            case TECHNICALLYALAC_COOKIE_MAXFRAMEBYTES: {
                if(technicallyalac_bitwriter_add(&f->bw,32,technicallyalac_max_packet_size(f))) {
                    f->si_state.state = TECHNICALLYALAC_COOKIE_AVGBITRATE;
                }
                break;
            }
            case TECHNICALLYALAC_COOKIE_AVGBITRATE: {
                if(technicallyalac_bitwriter_add(&f->bw,32,f->samplerate * f->channels * f->bitdepth)) {
                    f->si_state.state = TECHNICALLYALAC_COOKIE_SAMPLERATE;
                }
                break;
            }
            case TECHNICALLYALAC_COOKIE_SAMPLERATE: {
                if(technicallyalac_bitwriter_add(&f->bw,32,f->samplerate)) {
                    f->si_state.state = TECHNICALLYALAC_COOKIE_END;
                }
                break;
            }
            case TECHNICALLYALAC_COOKIE_END: {
                if(f->bw.bits == 0) {
                    r = 0;
                    f->si_state.state = TECHNICALLYALAC_COOKIE_START;
                }
                break;
            }

        }
    }

    assert(f->bw.pos > 0);
    *bytes = f->bw.pos;
    return r;
}

static int technicallyalac_channel(technicallyalac *f, uint32_t num_frames, int32_t *frames) {
    int r = 1;
    while(f->bw.pos < f->bw.len && r) {
        technicallyalac_bitwriter_flush(&f->bw);
        switch(f->ch_state.state) {
            case TECHNICALLYALAC_CHANNEL_START: {
                f->ch_state.frame = 0;
                f->ch_state.state = TECHNICALLYALAC_CHANNEL_CHANMAP;
                break;
            }
            case TECHNICALLYALAC_CHANNEL_CHANMAP: {
                if(technicallyalac_bitwriter_add(&f->bw,3,0)) {
                    f->ch_state.state = TECHNICALLYALAC_CHANNEL_TAG;
                }
                break;
            }
            case TECHNICALLYALAC_CHANNEL_TAG: {
                if(technicallyalac_bitwriter_add(&f->bw,4,f->pa_state.channel)) {
                    f->ch_state.state = TECHNICALLYALAC_CHANNEL_HEADER_BITS;
                }
                break;
            }
            case TECHNICALLYALAC_CHANNEL_HEADER_BITS: {
                if(technicallyalac_bitwriter_add(&f->bw,12,0)) {
                    f->ch_state.state = TECHNICALLYALAC_CHANNEL_SAMPLE_COUNT;
                }
                break;
            }
            case TECHNICALLYALAC_CHANNEL_SAMPLE_COUNT: {
                if(technicallyalac_bitwriter_add(&f->bw,1,num_frames != f->framelength)) {
                    f->ch_state.state = TECHNICALLYALAC_CHANNEL_EXTRA_BITS;
                }
                break;
            }
            case TECHNICALLYALAC_CHANNEL_EXTRA_BITS: {
                if(technicallyalac_bitwriter_add(&f->bw,2,0)) {
                    f->ch_state.state = TECHNICALLYALAC_CHANNEL_ESCAPE;
                }
                break;
            }
            case TECHNICALLYALAC_CHANNEL_ESCAPE: {
                if(technicallyalac_bitwriter_add(&f->bw,1,1)) {
                    f->ch_state.state = TECHNICALLYALAC_CHANNEL_DATA;
                    if(num_frames != f->framelength) {
                        f->ch_state.state = TECHNICALLYALAC_CHANNEL_SIZE;
                    }
                }
                break;
            }
            case TECHNICALLYALAC_CHANNEL_SIZE: {
                if(technicallyalac_bitwriter_add(&f->bw,32,num_frames)) {
                    f->ch_state.state = TECHNICALLYALAC_CHANNEL_DATA;
                }
                break;
            }
            case TECHNICALLYALAC_CHANNEL_DATA: {
                if(technicallyalac_bitwriter_add(&f->bw,f->bitdepth,frames[f->ch_state.frame])) {
                    f->ch_state.frame++;
                    if(f->ch_state.frame == num_frames) {
                        f->ch_state.state = TECHNICALLYALAC_CHANNEL_START;
                        r = 0;
                    }
                }
                break;
            }
        }
    }
    return r;
}

int technicallyalac_packet(technicallyalac *f, uint8_t *output, uint32_t *bytes, uint32_t num_frames, int32_t **frames) {
    int r = 1;

    f->bw.buffer = output;
    f->bw.len = *bytes;
    f->bw.pos = 0;

    while(f->bw.pos < f->bw.len && r) {
        technicallyalac_bitwriter_flush(&f->bw);
        switch(f->pa_state.state) {
            case TECHNICALLYALAC_PACKET_START: {
                f->pa_state.state = TECHNICALLYALAC_PACKET_CHANNEL;
                f->pa_state.channel = 0;

                f->ch_state.state = TECHNICALLYALAC_CHANNEL_START;
                break;
            }
            case TECHNICALLYALAC_PACKET_CHANNEL: {
                if(technicallyalac_channel(f,num_frames,frames[f->pa_state.channel]) == 0) {
                    f->pa_state.channel++;
                    if(f->pa_state.channel == f->channels) {
                        f->pa_state.state = TECHNICALLYALAC_PACKET_END;
                    }
                }
                break;
            }
            case TECHNICALLYALAC_PACKET_END: {
                if(technicallyalac_bitwriter_add(&f->bw,3,7)) {
                    technicallyalac_bitwriter_align(&f->bw);
                    f->pa_state.state = TECHNICALLYALAC_PACKET_FLUSH;
                }
                break;
            }
            case TECHNICALLYALAC_PACKET_FLUSH: {
                if(f->bw.bits == 0) {
                    f->pa_state.state = TECHNICALLYALAC_PACKET_START;
                    r = 0;
                }
                break;
            }
        }
    }

    return r;

}

static uint64_t technicallyalac_packet_size_internal(technicallyalac *f) {
    uint64_t bits = 0;
    bits +=  3; /* channel tag */
    bits +=  4; /* channel number */
    bits += 12; /* header bits, all zero */
    bits +=  1; /* sample count flag */
    bits +=  2; /* extra bits */
    bits +=  1; /* channel escape */

    bits += (uint64_t)f->bitdepth * (uint64_t)f->framelength; /* raw bits in a frame */

    bits *= (uint64_t)f->channels;

    bits += 3; /* ID_END tag */
    return bits;

}

uint32_t technicallyalac_packet_size(technicallyalac *f) {
    uint64_t bits = technicallyalac_packet_size_internal(f);
    uint64_t rem = bits % 8;
    if(rem > 0) bits += 8 - rem;
    bits /= 8;
    return (uint32_t)bits;
}

uint32_t technicallyalac_max_packet_size(technicallyalac *f) {
    uint64_t rem = 0;
    uint64_t bits = technicallyalac_packet_size_internal(f);
    uint64_t max = bits;

    /* assuming the final block is 1 sample short */
    max -= f->bitdepth + f->channels;
    max += 32;
    bits = ( max > bits ? max : bits );

    rem = bits % 8;
    if(rem > 0) bits += 8 - rem;
    bits /= 8;

    return (uint32_t)bits;
}

#endif
