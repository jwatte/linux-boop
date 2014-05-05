
#include <alsa/asoundlib.h>
#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>

#include <iostream>


#define LRAND_MAX 0x80000000


/* lrand is better than regular rand, they say */
float rfloat() {
    return (float)lrand48() / LRAND_MAX;
}

/* parameters for the PD implemented FM wave */
struct Parameters {
    float phase1;
    float phase2;
    float phase3;
    float v1;
    float v2;
    float v3;
};

/* generate a sample array based on parameters */
void generate_sample(Parameters const &pa, short *buf, size_t cnt) {
    float phase1 = pa.phase1;
    float phase2 = pa.phase2;
    float phase3 = pa.phase3;
    float v1 = pa.v1;
    float v2 = pa.v2;
    float v3 = pa.v3;
    float p = 0;
    float p2 = 0;
    float p3 = 0;
    /* the structure implemented is:
          1 <+- 2 <+- 3
             |     |
             +-----+
       Oscillator 3 is best use as a LFO rather than signal-frequency 
       modulator.
     */

    while (cnt != 0) {
        *buf = (short)(::sinf(p) * 32767);
        ++buf;
        p += phase1 + ::sinf(p2) * v2 + ::sinf(p3) * v1;
        p2 += phase2 + ::sinf(p3) * v3;
        p3 += phase3;
        --cnt;
    }
}

/* generate random parameters */
Parameters random_parameters() {
    Parameters ret = { 0 };
    /* phase1 is the carrier inverse frequency */
    ret.phase1 = rfloat() * 0.3f + 0.15f;
    /* phase2 is the modulator inverse frequency */
    ret.phase2 = rfloat() * 0.5f + 0.1f;
    /* phase3 is the LFO inverser frequency */
    ret.phase3 = rfloat() * 0.01f + 0.00007f;
    /* v1 is LFO modulating carrier */
    ret.v1 = rfloat() * 0.02f + 0.005f;
    /* v2 is modulator modulating carrier */
    ret.v2 = rfloat() * 0.5f + 0.3f;
    /* v3 is LFO modulating modulator */
    ret.v3 = rfloat() * 0.1f + 0.05f;
    return ret;
}


/* generate a random sample */
void generate(short *buf, size_t cnt) {
    /* set up parameters */
    generate_sample(random_parameters(), buf, cnt);
}

int main(int argc, char const *argv[]) {

    int err;
    snd_pcm_t *devh;
    snd_pcm_hw_params_t *hwp;

    /* parse command line arguments */
    float len = 0.1f;
    int num = 3;
    if (argc > 1) {
        len = atof(argv[1]);
        if (len < 0.01f || len > 10) {
            goto usage;
        }
        num = 0;
        ++argv;
        --argc;
    }
    if (argc > 1) {
        num = atoi(argv[1]);
        if (num < 1 || num > 10) {
            goto usage;
        }
        ++argv;
        --argc;
    }
    if (argc > 2 || (argc == 2 && argv[1][0] == '-')) {
usage:
        fprintf(stderr, "usage: boop [len [num [hw:#,3]]]\n");
        fprintf(stderr, "length: 0.01 to 10\n");
        fprintf(stderr, "num: 1 to 10\n");
        exit(1);
    }

    /* generate a sufficiently long boop that's different each time */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand48(ts.tv_sec ^ ts.tv_nsec);

    size_t count = (size_t)(len * 44100);
    short *data = (short *)calloc(count * num, 2);
    for (int i = 0; i != num; ++i) {
        generate(data + i * count, count);
    }
    count *= num;

    /* which hardware are we playing to? */
    char const *devname = (argv[1] ? argv[1] : "plughw:0,0");
    if ((err = snd_pcm_open(&devh, devname, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        std::cerr << "snd_pcm_open(" << devname << ") failed: " << snd_strerror(err) << std::endl;
        exit(1);
    }

    /* Set up the hardware parameters. This is tedious. */
    if ((err = snd_pcm_hw_params_malloc(&hwp)) < 0) {
        std::cerr << "snd_pcm_hw_params_malloc() failed: " << snd_strerror(err) <<std::endl;
        exit(1);
    }
    if ((err = snd_pcm_hw_params_any(devh, hwp)) < 0) {
        std::cerr << "snd_pcm_hw_params_any() failed: " << snd_strerror(err) <<std::endl;
        exit(1);
    }
    if ((err = snd_pcm_hw_params_set_access(devh, hwp, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        std::cerr << "snd_pcm_hw_params_set_access() failed: " << snd_strerror(err) <<std::endl;
        exit(1);
    }
    unsigned int rate = 44100;
    int dir = -1;
    if ((err = snd_pcm_hw_params_set_rate_near(devh, hwp, &rate, &dir)) < 0) {
        std::cerr << "snd_pcm_hw_params_set_rate_near(44100) failed: " << snd_strerror(err) <<std::endl;
        exit(1);
    }
    if ((err = snd_pcm_hw_params_set_channels(devh, hwp, 1)) < 0) {
        std::cerr << "snd_pcm_hw_params_set_channels(1) failed: " << snd_strerror(err) <<std::endl;
        exit(1);
    }
    if ((err = snd_pcm_hw_params_set_format(devh, hwp, SND_PCM_FORMAT_S16_LE)) < 0) {
        std::cerr << "snd_pcm_hw_params_set_format() failed: " << snd_strerror(err) <<std::endl;
        exit(1);
    }
    if ((err = snd_pcm_hw_params_set_periods_integer(devh, hwp)) < 0) {
        std::cerr << "snd_pcm_hw_params_set_integer() failed: " << snd_strerror(err) <<std::endl;
        exit(1);
    }
    if ((err = snd_pcm_hw_params_set_buffer_size(devh, hwp, 4096)) < 0) {
        std::cerr << "snd_pcm_hw_params_set_buffer_size(4096) failed: " << snd_strerror(err) <<std::endl;
        exit(1);
    }
    if ((err = snd_pcm_hw_params(devh, hwp)) < 0) {
        std::cerr << "snd_pcm_hw_params() failed: " << snd_strerror(err) <<std::endl;
        exit(1);
    }
    snd_pcm_hw_params_free(hwp);

    if ((err = snd_pcm_prepare(devh)) < 0) {
        perror("prepare() failed");
        exit(1);
    }

    /* play the wave in a few blocks */
    for (size_t i = 0; i < count;) {
        size_t tw = 4096;
        if (tw + i > count) {
            tw = count - i;
        }
        if ((err = snd_pcm_writei(devh, &data[i], tw)) < 0) {
            std::cerr << "snd_pcm_writei failed: " << snd_strerror(err) <<std::endl;
            exit(1);
        }
        i += tw;
    }
   
    /* play out and close */
    snd_pcm_drain(devh);
    snd_pcm_close(devh);

    exit(0);
}

