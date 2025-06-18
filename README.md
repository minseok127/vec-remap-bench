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
