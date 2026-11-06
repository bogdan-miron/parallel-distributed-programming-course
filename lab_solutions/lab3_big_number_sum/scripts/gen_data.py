# makes Numar1.txt and Numar2.txt for one test case, and expected.txt with the sum made by python
# the files have the number of digits, then the digits, least significant first (position 0 first)
# usage: python3 gen_data.py <out_dir> <n1> <n2>
#        python3 gen_data.py <out_dir> fixed      the 16 digit numbers from the statement
#        python3 gen_data.py <out_dir> nines <n>  n digits of 9 plus the number 1, the carry goes through all the digits
import random
import sys

out_dir = sys.argv[1]

# python limits the size of int <-> str conversions, the numbers here are longer
if hasattr(sys, "set_int_max_str_digits"):
    sys.set_int_max_str_digits(0)


def digits_of(s):
    # s is the number as it is read, the first digit of the list is the last character
    return [int(c) for c in reversed(s)]


def write_number(path, d):
    with open(path, "w") as f:
        f.write(f"{len(d)}\n")
        f.write("".join(str(x) for x in d) + "\n")


if sys.argv[2] == "fixed":
    a = digits_of("9999444444449999")
    b = digits_of("9999555555559999")
elif sys.argv[2] == "nines":
    n = int(sys.argv[3])
    a = [9] * n
    b = [1]
else:
    n1, n2 = int(sys.argv[2]), int(sys.argv[3])
    rnd = random.Random(n1 * 100003 + n2)
    a = [rnd.randint(0, 9) for _ in range(n1)]
    b = [rnd.randint(0, 9) for _ in range(n2)]
    # the most significant digit is not 0
    a[-1] = rnd.randint(1, 9)
    b[-1] = rnd.randint(1, 9)

write_number(out_dir + "/Numar1.txt", a)
write_number(out_dir + "/Numar2.txt", b)

x = int("".join(str(d) for d in reversed(a)))
y = int("".join(str(d) for d in reversed(b)))
with open(out_dir + "/expected.txt", "w") as f:
    f.write(str(x + y)[::-1] + "\n")
