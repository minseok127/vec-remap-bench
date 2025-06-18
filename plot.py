import csv
import sys
import math
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter, LogLocator

def human_bytes(x, _):
    """4 KiB → 4 KiB, 1 MiB → 1 MiB … friendly tick labels."""
    if x == 0:
        return "0"
    units = ["B", "KiB", "MiB", "GiB", "TiB"]
    k = int(math.log(x, 1024))
    val = x / (1024 ** k)
    # use integer for powers of two, else one decimal
    txt = f"{val:.0f}" if val.is_integer() else f"{val:.1f}"
    return f"{txt} {units[k]}"

def main(path):
    data = {}
    with open(path) as f:
        for row in csv.DictReader(f):
            size  = int(row["size"])
            meth  = row["method"]
            t     = int(row["ns"])
            data.setdefault(meth, []).append((size, t))

    for meth, pts in data.items():
        pts.sort()
        sizes  = [s for s, _ in pts]
        times  = [t for _, t in pts]
        plt.plot(sizes, times, label=meth, marker="o")

    ax = plt.gca()
    ax.set_xscale("log", base=2)                     # powers-of-two grid
    ax.xaxis.set_major_locator(LogLocator(base=2))   # 4 KiB, 8 KiB, …
    ax.xaxis.set_major_formatter(FuncFormatter(human_bytes))

    plt.xlabel("size")
    plt.ylabel("extend time (ns)")
    plt.title("memcpy vs. mremap – extend latency")
    plt.legend()
    plt.tight_layout()
    plt.savefig("result.png", dpi=150)

if __name__ == "__main__":
    csv_path = sys.argv[1] if len(sys.argv) > 1 else "result.csv"
    main(csv_path)

