#include "example-shared.h"

#define TECHNICALLYALAC_IMPLEMENTATION
#include "../technicallyalac.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* example that reads in a headerless WAV file and writes
 * out a CAF file with ALAC data. assumes WAV is 16-bit, 2channel, 44100Hz */

/* headerless wav can be created via ffmpeg like:
 *     ffmpeg -i your-audio.mp3 -ar 44100 -ac 2 -f s16le your-audio.raw
 */

/* example assumes we're on a little-endian system - would need to have
 * a proper decoder for input WAV data. */

#define BUFFER_SIZE 1
#define FRAMELENGTH 4096

int main(int argc, const char *argv[]) {
    uint8_t buffer[BUFFER_SIZE];
    uint32_t bufferlen = BUFFER_SIZE;
    FILE *input;
    FILE *output;
    uint32_t frames;
    int16_t *raw_samples;
    int32_t *samples[2];
    int32_t *samplesbuf;
    size_t pos = 0;
    size_t len = 0;
    uint64_t total = 0;
    uint32_t remainder = 0;
    size_t packetsize = 0;
    technicallyalac f;

    if(argc < 3) {
        printf("Usage: %s /path/to/raw /path/to/caf\n",argv[0]);
        return 1;
    }

    input = fopen(argv[1],"rb");
    if(input == NULL) return 1;

    output = fopen(argv[2],"wb");
    if(output == NULL) {
        fclose(input);
        return 1;
    }

    fwrite("caff",1,4,output);
    write_uint16(output,1); /* file version */
    write_uint16(output,0); /* file flags */

    technicallyalac_init(&f,FRAMELENGTH,44100,2,16);

    raw_samples = (int16_t *)malloc(sizeof(int16_t) * f.channels * f.framelength);
    if(!raw_samples) abort();
    samplesbuf = (int32_t *)malloc(sizeof(int32_t) * f.channels * f.framelength);
    if(!samplesbuf) abort();
    samples[0] = &samplesbuf[0];
    samples[1] = &samplesbuf[f.framelength];

    packetsize = technicallyalac_packet_size(&f);

    /* begin audio description chunk */
    fwrite("desc",1,4,output); /* chunk type */
    write_uint64(output,32); /* size of chunk */
    write_double(output,44100.0); /* sample rate */
    fwrite("alac",1,4,output); /* format id */
    write_uint32(output,0); /* format flags */
    write_uint32(output,packetsize); /* bytes per packet */
    write_uint32(output,FRAMELENGTH); /* frames per packet */
    write_uint32(output, 2); /* channels per frame */
    write_uint32(output,16); /* bits per channel */
    /* end audio description chunk */

    /* begin cookie chunk */
    fwrite("kuki",1,4,output); /* chunk type */
    write_uint64(output,technicallyalac_size_cookie()); /* chunk size */

    while(technicallyalac_cookie(&f,buffer,&bufferlen)) {
        fwrite(buffer,1,bufferlen,output);
        bufferlen = BUFFER_SIZE;
    }
    fwrite(buffer,1,bufferlen,output);
    /* end cookie chunk */

    bufferlen = BUFFER_SIZE;

    /* begin data chunk */
    fwrite("data",1,4,output); /* chunk type */
    pos = ftell(output); /* need to rewind here when we're done */
    write_uint64(output,0); /* write a temp value for now, will update at end */

    write_uint32(output,0); /* edit count */

    /* fun fact about CAF, if you use variable packet sizes, you have to
     * include a packet table, and it's fairly complicated. But the packet
     * table lets you say "trim (x) frames off the final packet", so we'll
     * just make all the packets the same length and use that trim feature
     * at the end, to avoid having to do the entire table */

    memset(raw_samples,0,sizeof(int16_t) * 2 * f.framelength);
    while((frames = fread(raw_samples,sizeof(int16_t) * 2, f.framelength, input)) > 0) {
        repack_samples_deinterleave(samples,raw_samples,2,frames, 0);

        while(technicallyalac_packet(&f,buffer,&bufferlen,f.framelength,samples)) {
            fwrite(buffer,1,bufferlen,output);
            bufferlen = BUFFER_SIZE;
        }
        fwrite(buffer,1,bufferlen,output);
        bufferlen = BUFFER_SIZE;

        memset(raw_samples,0,sizeof(int16_t) * 2 * f.framelength);
        total += f.framelength;
        remainder = f.framelength - frames;
    }

    len = ftell(output); /* store our current position */
    fseek(output,pos,SEEK_SET); /* rewind to the data chunk length */
    write_uint64(output,len-pos-8); /* write out the length */
    fseek(output,0,SEEK_END);
    /* end data chunk */

    /* begin packet table */
    fwrite("pakt",1,4,output); /* chunk type */
    write_uint64(output,24); /* chucnk size */
    write_uint64(output,0); /* number of packets in the table (0, we have no table) */
    write_uint64(output,total); /* number of encoded frames */
    write_uint32(output,0); /* priming frames (ALAC doesn't need any priming) */
    write_uint32(output,remainder); /* remainder frames */
    /* end packet table */


    fclose(input);
    fclose(output);
    quit(0,raw_samples,samplesbuf, NULL);

    return 0;
}


