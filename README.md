# vec-remap-bench

Benchmark comparing vector extension using memcpy vs page table remap.

## Build

```
make benchmark
```

## Run benchmark

```
./benchmark
```

This produces `result.csv`. To visualize:

```
python3 plot.py result.csv
```

The plot is saved as `result.png`.
