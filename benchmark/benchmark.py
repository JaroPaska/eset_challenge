import os
import random
import subprocess
import sys
import time

random.seed(0)
n = 1

for i in range(n):
    filepath = f'files/{i}.in'
    if not os.path.exists(filepath):
        print(f'Writing {filepath}')
        with open(filepath, 'w') as f:
            f.write(''.join([str(random.randint(0, 1)) for _ in range(100_000_000)]))

out = open('out', 'w')
print('Running minigrep')
t0 = time.time()
subprocess.run([sys.argv[1], '.', '111'], stdout=out)
out.close()
print(f'{time.time() - t0} seconds elapsed')
