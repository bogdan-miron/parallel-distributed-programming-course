# makes the input files for one test case
# usage: python3 gen_data.py <out_dir> <args of the test case>
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools"))
from gen_lib import write_matrix, write_vector  # noqa: E402,F401

out_dir = sys.argv[1]

# TODO read the arguments of the case and write the input files from the statement
# for example: write_matrix(out_dir + "/MatrixF.txt", n, m, 0, 255)
