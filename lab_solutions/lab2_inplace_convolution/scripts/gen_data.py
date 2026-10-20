# makes date.txt for one test case: the matrix F (n m on the first line) and then the 3 x 3 kernel
# usage: python3 gen_data.py <out_dir> <n> <m>
import os
import random
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools"))
from gen_lib import write_matrix  # noqa: E402

out_dir = sys.argv[1]
n, m = int(sys.argv[2]), int(sys.argv[3])
path = f"{out_dir}/date.txt"

# same sizes give the same file
write_matrix(path, n, m, 0, 255, seed=n * 100003 + m)

rnd = random.Random(3)
with open(path, "a") as f:
    f.write("3 3\n")
    for _ in range(3):
        f.write(" ".join(str(rnd.randint(-3, 3)) for _ in range(3)) + "\n")
