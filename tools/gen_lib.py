# helpers for the gen_data.py of a lab, use what the statement needs and write the rest by hand
# files start with the sizes on the first line, then the numbers (change header=False if the statement says otherwise)
import random


def write_matrix(path, n, m, lo, hi, real=False, decimals=4, seed=0, header=True):
    """n x m numbers between lo and hi, one row per line. real=False gives integers."""
    try:
        import numpy as np
    except ImportError:
        np = None
    with open(path, "w") as f:
        if header:
            f.write(f"{n} {m}\n")
        if np is None:
            rnd = random.Random(seed)
            for _ in range(n):
                if real:
                    row = (f"{rnd.uniform(lo, hi):.{decimals}f}" for _ in range(m))
                else:
                    row = (str(rnd.randint(lo, hi)) for _ in range(m))
                f.write(" ".join(row) + "\n")
            return
        rng = np.random.default_rng(seed)
        for start in range(0, n, 1000):  # in chunks, so a big matrix does not fill the memory
            rows = min(1000, n - start)
            if real:
                a = rng.uniform(lo, hi, (rows, m))
                np.savetxt(f, a, fmt=f"%.{decimals}f")
            else:
                a = rng.integers(lo, hi + 1, (rows, m))
                np.savetxt(f, a, fmt="%d")


def write_vector(path, n, lo, hi, real=False, decimals=4, seed=0):
    """n numbers, first line is n, then one number per line."""
    rnd = random.Random(seed)
    with open(path, "w") as f:
        f.write(f"{n}\n")
        for _ in range(n):
            f.write((f"{rnd.uniform(lo, hi):.{decimals}f}" if real else str(rnd.randint(lo, hi))) + "\n")
