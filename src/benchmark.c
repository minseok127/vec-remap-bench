/* bench_pte_vs_memcpy_en.c
 *
 * Fair benchmark: memory copy (memcpy) vs PTE move (mremap).
 *
 * Output CSV: size,method,ns
 * Format rules: tab = 4 spaces, line ≤ 80 chars, braces for single-liners.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

/* nanoseconds between two timespec values */
static inline long nsec_diff(struct timespec a, struct timespec b)
{
    return (b.tv_sec - a.tv_sec) * 1000000000L + (b.tv_nsec - a.tv_nsec);
}

/* touch every page to force physical allocation */
static void fault_in(char *p, size_t len)
{
    const size_t ps = sysconf(_SC_PAGESIZE);
    for (size_t i = 0; i < len; i += ps) { p[i] = 0; }
}

/*--------------------------- memcpy path -----------------------------------*/
static void bench_memcpy(size_t sz, FILE *out)
{
    /* map 2× region so src and dst are contiguous */
    char *region = mmap(NULL, sz * 2, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (region == MAP_FAILED) { perror("mmap memcpy"); return; }

    char *src = region;
    char *dst = region + sz;
    fault_in(src, sz);
    fault_in(dst, sz);

    struct timespec s, e;
    clock_gettime(CLOCK_MONOTONIC, &s);
    memcpy(dst, src, sz);
    clock_gettime(CLOCK_MONOTONIC, &e);

    if (out) { fprintf(out, "%zu,memcpy,%ld\n", sz, nsec_diff(s, e)); }
    munmap(region, sz * 2);
}

/*--------------------------- mremap path -----------------------------------*/
static void bench_mremap(size_t sz, FILE *out)
{
    /* 1) original region */
    char *old = mmap(NULL, sz, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (old == MAP_FAILED) { perror("mmap old"); return; }
    fault_in(old, sz);

    /* 2) reserve destination, 3) immediately unmap to leave a hole */
    char *dest = mmap(NULL, sz * 2, PROT_NONE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (dest == MAP_FAILED) { perror("mmap dest"); munmap(old, sz); return; }
    munmap(dest, sz * 2);   /* dest range is now guaranteed empty */

    /* 4) move PTEs without copying */
    struct timespec s, e;
    clock_gettime(CLOCK_MONOTONIC, &s);
    void *newp = mremap(old, sz, sz * 2,
                        MREMAP_MAYMOVE | MREMAP_FIXED, dest);
    clock_gettime(CLOCK_MONOTONIC, &e);
    if (newp == MAP_FAILED) {
        perror("mremap");
        munmap(old, sz);
        return;
    }

    /* grant RW and touch the second half (outside timing window) */
    mprotect((char *)newp + sz, sz, PROT_READ | PROT_WRITE);
    fault_in((char *)newp + sz, sz);

    if (out) { fprintf(out, "%zu,mremap,%ld\n", sz, nsec_diff(s, e)); }
    munmap(newp, sz * 2);
}

/*---------------------------- driver ---------------------------------------*/
int main(void)
{
    const size_t pg = sysconf(_SC_PAGESIZE);   /* usually 4096 */
    FILE *f = fopen("result.csv", "w");
    if (!f) { perror("fopen"); return 1; }
    fprintf(f, "size,method,ns\n");

    for (size_t sz = pg; sz <= 1 << 27; sz <<= 1) {
        /* warm-up (not recorded) */
        bench_memcpy(sz, NULL);
        bench_mremap(sz, NULL);

        for (int i = 0; i < 5; i++) {
            bench_memcpy(sz, f);
            bench_mremap(sz, f);
        }
        fflush(f);
    }
    fclose(f);
    return 0;
}

