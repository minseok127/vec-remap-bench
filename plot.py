import csv
import sys
import matplotlib.pyplot as plt

def main(path):
	data = {}
	with open(path) as f:
	    r = csv.DictReader(f)
	    for row in r:
	        size = int(row['size'])
	        method = row['method']
	        time = int(row['ns'])
	        data.setdefault(method, []).append((size, time))
	for m in data:
	    data[m].sort()
	for method, points in data.items():
	    sizes = [s for s, _ in points]
	    times = [t for _, t in points]
	    plt.plot(sizes, times, label=method)
	plt.xlabel('size (bytes)')
	plt.ylabel('extend time (ns)')
	plt.xscale('log')
	plt.legend()
	plt.tight_layout()
	plt.savefig('result.png')

if __name__ == '__main__':
	path = sys.argv[1] if len(sys.argv) > 1 else 'result.csv'
	main(path)
