# computes the convolution straight from the definition and compares it with output_seq.txt
# usage: python3 reference.py <folder with MatrixF.txt, ConvolutionM.txt and output_seq.txt>   (small matrices only)
import sys

d = sys.argv[1]


def read(path):
    rows = [list(map(int, line.split())) for line in open(path)]
    return rows[0], rows[1:]


(n, m), f = read(d + "/MatrixF.txt")
(k, _), c = read(d + "/ConvolutionM.txt")
_, out = read(d + "/output_seq.txt")
r = k // 2

for i in range(n):
    for j in range(m):
        s = 0
        for a in range(k):
            for b in range(k):
                s += f[min(max(i + a - r, 0), n - 1)][min(max(j + b - r, 0), m - 1)] * c[a][b]
        if s != out[i][j]:
            sys.exit(f"different at ({i}, {j}): reference {s}, output_seq.txt {out[i][j]}")
print("output_seq.txt is the same as the reference result")
