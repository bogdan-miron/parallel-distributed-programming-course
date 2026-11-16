# makes project1.txt ... project10.txt for one test case, and expected.txt with the final grades made by python
# every line of a project file is "id grade", a file has between 70% and 100% of the students (70 to 100 for 100 students),
# in a different random order in every file, grades are integers from 1 to 10
# usage: python3 gen_data.py <out_dir> <number of students>
import random
import sys

out_dir = sys.argv[1]
students = int(sys.argv[2])
rnd = random.Random(students)

ids = rnd.sample(range(1000, 100000), students)
total = {}
for f in range(1, 11):
    count = rnd.randint(students * 70 // 100, students)
    chosen = rnd.sample(ids, count)
    with open(f"{out_dir}/project{f}.txt", "w") as out:
        for i in chosen:
            g = rnd.randint(1, 10)
            out.write(f"{i} {g}\n")
            total[i] = total.get(i, 0) + g

# the final grade is the sum divided by 10, with one decimal, the ids in increasing order
with open(f"{out_dir}/expected.txt", "w") as out:
    for i in sorted(total):
        out.write(f"{i} {total[i] // 10}.{total[i] % 10}\n")
