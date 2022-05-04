import random
random.seed(0)
n = 1

print('Writing test files')
for i in range(n):
    with open(f'{i}.in', 'w') as f:
        f.write(''.join([str(random.randint(0, 1)) for _ in range(100000000)]))

print('Running minigrep')
import subprocess, sys, time
t0 = time.time()
out = open('out', 'w')
subprocess.run([sys.argv[1], '.', '111'], stdout=out)
out.close()
print(f'{time.time() - t0} seconds elapsed')

print('Removing files')
import os
for i in range(n):
    os.remove(f'{i}.in')
os.remove('out')