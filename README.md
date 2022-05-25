# technicallyalac

A single-file C library for creating Apple Lossless Audio Codec (ALAC) streams. Does not use any C library functions,
does not allocate any heap memory.

The streams are not compressed, hence the name "technically" ALAC.

## Usage

In one C file define `TECHNICALLYALAC_IMPLEMENTATION` before including `technicallyalac.h`.

See `technicallyalac.h` for details on how to use the library, also see `examples/example-caf.c`. Most applications will probably do something like:

```C

#define BUFFER_LEN 8192 /* some buffer length */
#define FRAME_LENGTH 1024 /* frame length to use in each packet */
technicallyalac a;
uint8_t buffer[BUFFER_LEN];
uint32_t bufferlen = BUFFER_LEN;

technicallyalac_init(&a, FRAME_LENGTH, 44100, 2, 16);

/* check that our buffer is large enough for packets */
assert(BUFFER_LEN >= technicallyalac_max_packet_size(&a));

/* get the "magic cookie" data (ffmpeg calls this "extradata") */
technicallyalac_cookie(&a, buffer, &bufferlen);

/* cookie is in buffer, save it somewhere */

/* assuming:
  blocks is a int32_t **,
  first dimension is channels, second dimension is samples
  so for a stereo file you would have
  blocks[2][1024]

  then blocks_of_audio == total number of blocks
*/
for(i=0;i<blocks_of_audio;i++) {
    /* reset our buffer length */
    bufferlen = BUFFER_LEN;
    technicallyalac_packet(&f,buffer,&bufferlen,blocks[i].size,blocks[i].data);

    /* just write out the buffer for now - in reality this
    requires some kind of framing, see example-caf.c for
    writing to a Core Audio Format file. */
    fwrite(buffer,1,bufferlen,output);
}
```

## LICENSE

BSD Zero Clause (see the `LICENSE` file).

