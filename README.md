# PTE-Move vs. Memory-Copy Benchmark

This micro-benchmark measures the latency difference between

1. **Memory copying** with `memcpy`, and  
2. **Page-table relocation** with `mremap` (moving the same physical pages to a
   new virtual address—zero data copied).

---

## Files

| File | Purpose |
| ---- | ------- |
| `src/benchmark.c` | C source of the benchmark |
| `result.csv` | Generated after a run; raw timings |

`result.csv` columns:

| Column | Meaning |
| ------ | ------- |
| `size`   | Bytes in the **original** mapping (the test always doubles it) |
| `method` | `memcpy` or `mremap` |
| `ns`     | Elapsed nanoseconds for one operation |

---

## How the benchmark works

| Step | **memcpy** path | **mremap** path |
| ---- | --------------- | --------------- |
| Map memory | Map one 2 × `size` region; split into **src/dst** | Map **old** region (`size` bytes) |
| Fault-in   | Touch every page in **src** and **dst** | Touch every page in **old** |
| Reserve dest | — | Map a 2 × `size` **dest** region, then *immediately* `munmap` it → leaves an empty hole |
| **Timed section** | `memcpy(dst, src, size)` | `mremap(old → dest, 2×size, MREMAP_FIXED)` (PTE move only) |
| Post-work  | — | Grant RW to new half, touch pages (outside timer) |
| Cleanup    | `munmap` | `munmap` |

Fairness safeguards:

* Sizes are **page-aligned** (`PAGE_SIZE` multiples).  
* Both paths fault-in their destination pages → identical page-fault load.  
* One **warm-up run** per size (not recorded) removes cold-cache noise.  
* Dest range is guaranteed empty → kernel must follow the “move-without-copy”
  path, never erroring with `EINVAL`.

---

## Build & run

```bash
gcc -O2 -Wall -std=c11 bench_pte_vs_memcpy_en.c -o bench
./bench          # writes result.csv
```

---

## Environment

- Hardware
	- CPU: Intel Core i5-13400F (16 cores)
	- RAM: 16GB DDR5 5600MHz

- Software
	- OS: Ubuntu 24.04.1 LTS
	- Compiler: GCC 13.3.0
	- Build System: GNU Make 4.3

 ---

 ## Result

|      Size | memcpy median (ns) | mremap median (ns) | Speed-up × (memcpy / mremap) |
| --------: | -----------------: | -----------------: | ---------------------------: |
|   4.0 KiB |                 73 |              2 111 |                         0.03 |
|   8.0 KiB |                 89 |              2 111 |                         0.04 |
|  16.0 KiB |                205 |              2 539 |                         0.08 |
|  32.0 KiB |                980 |              3 239 |                         0.30 |
|  64.0 KiB |              1 911 |              4 479 |                         0.43 |
| 128.0 KiB |              3 707 |              6 759 |                         0.55 |
| 256.0 KiB |              5 967 |              1 474 |                         4.05 |
| 512.0 KiB |             10 626 |              1 810 |                         5.87 |
|   1.0 MiB |             36 476 |              3 486 |                        10.46 |
|   2.0 MiB |             77 747 |              7 084 |                        10.98 |
|   4.0 MiB |            154 627 |              5 821 |                        26.56 |
|   8.0 MiB |            364 247 |              6 944 |                        52.45 |
|  16.0 MiB |            880 624 |              9 899 |                        88.96 |
|  32.0 MiB |          2 505 903 |             12 209 |                       205.25 |
|  64.0 MiB |          5 151 014 |             15 932 |                       323.31 |
| 128.0 MiB |          8 885 021 |             17 815 |                       498.74 |
