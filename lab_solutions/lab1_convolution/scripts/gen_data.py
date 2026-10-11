# makes MatrixF.txt and ConvolutionM.txt for one test case
# usage: python3 gen_data.py <out_dir> <n> <m> <k>
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools"))
from gen_lib import write_matrix  # noqa: E402

out_dir = sys.argv[1]
n, m, k = int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4])

# same sizes give the same files, first line of each file is the size
write_matrix(f"{out_dir}/MatrixF.txt", n, m, 0, 255, seed=n * 100003 + m)
write_matrix(f"{out_dir}/ConvolutionM.txt", k, k, -3, 3, seed=k)
