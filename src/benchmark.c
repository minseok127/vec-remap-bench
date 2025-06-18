#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>

static long timediff_ns(struct timespec a, struct timespec b)
{
	return (b.tv_sec - a.tv_sec) * 1000000000L +
	(b.tv_nsec - a.tv_nsec);
}

static void bench_memcpy(size_t size, FILE *out)
{
	char *ptr = malloc(size);
	if (!ptr) {
	perror("malloc");
	return;
	}
	memset(ptr, 0, size);
	size_t new_size = size * 2;
	struct timespec s, e;
	clock_gettime(CLOCK_MONOTONIC, &s);
	char *new_ptr = malloc(new_size);
	memcpy(new_ptr, ptr, size);
	free(ptr);
	clock_gettime(CLOCK_MONOTONIC, &e);
	free(new_ptr);
	fprintf(out, "%zu,memcpy,%ld\n", size, timediff_ns(s, e));
}

static void bench_mremap(size_t size, FILE *out)
{
	char *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE,
	MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (ptr == MAP_FAILED) {
	perror("mmap");
	return;
	}
	memset(ptr, 0, size);
	size_t new_size = size * 2;
	struct timespec s, e;
	clock_gettime(CLOCK_MONOTONIC, &s);
	char *new_ptr = mremap(ptr, size, new_size, MREMAP_MAYMOVE);
	clock_gettime(CLOCK_MONOTONIC, &e);
	if (new_ptr == MAP_FAILED) {
	perror("mremap");
	munmap(ptr, size);
	return;
	}
	munmap(new_ptr, new_size);
	fprintf(out, "%zu,mremap,%ld\n", size, timediff_ns(s, e));
}

int main(void)
{
	FILE *f = fopen("result.csv", "w");
	if (!f) {
	perror("fopen");
	return 1;
	}
	fprintf(f, "size,method,ns\n");
	for (size_t sz = 1<<10; sz <= 1<<27; sz <<= 1) {
	for (int i = 0; i < 5; i++) {
	bench_memcpy(sz, f);
	bench_mremap(sz, f);
	}
	}
	fclose(f);
	return 0;
}
