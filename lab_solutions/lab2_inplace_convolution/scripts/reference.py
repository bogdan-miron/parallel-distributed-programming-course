# computes the filtered matrix straight from the definition and compares it with output_seq.txt
# usage: python3 reference.py <folder with date.txt and output_seq.txt>   (small matrices only)
import sys

d = sys.argv[1]


def read(path):
    rows = [list(map(int, line.split())) for line in open(path)]
    return rows


date = read(d + "/date.txt")
n, m = date[0]
f = date[1:1 + n]
c = date[2 + n:5 + n]
out = read(d + "/output_seq.txt")[1:]

for i in range(n):
    for j in range(m):
        s = 0
        for a in range(3):
            for b in range(3):
                s += f[min(max(i + a - 1, 0), n - 1)][min(max(j + b - 1, 0), m - 1)] * c[a][b]
        if s != out[i][j]:
            sys.exit(f"different at ({i}, {j}): reference {s}, output_seq.txt {out[i][j]}")
print("output_seq.txt is the same as the reference result")
