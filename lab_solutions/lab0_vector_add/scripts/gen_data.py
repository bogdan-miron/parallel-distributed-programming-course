# makes A.txt, B.txt and expected.txt (the sum, computed here) for the vector addition
# usage: python3 gen_data.py <out_dir> <n>
import random
import sys

out_dir = sys.argv[1]
n = int(sys.argv[2])
random.seed(n)  # same n gives the same files

a = [round(random.uniform(-1000, 1000), 4) for _ in range(n)]
b = [round(random.uniform(-1000, 1000), 4) for _ in range(n)]


def write(name, v, fmt):
    with open(f"{out_dir}/{name}", "w") as f:
        f.write(f"{len(v)}\n")
        f.write("\n".join(fmt % x for x in v))
        f.write("\n")


write("A.txt", a, "%.4f")
write("B.txt", b, "%.4f")
write("expected.txt", [x + y for x, y in zip(a, b)], "%.6f")
