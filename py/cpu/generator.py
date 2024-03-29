import random
from sys import argv

import numpy as np

from lib import m_hash_hex

seed = int(argv[3]) if len(argv) > 3 else random.randint(0, 2_000_000)
with_hash = True
if seed == 0:
    seed = random.randint(0, 2_000_000)
    with_hash = False
np.random.seed(seed)

matrix = np.random.randint(low=0, high=2, size=(int(argv[1]), int(argv[2])), dtype=int)

cols_sums = '[' + ','.join(str(e) for e in matrix.sum(0).tolist()) + ']'
rows_sums = '[' + ','.join(str(e) for e in matrix.sum(1).flatten().tolist()) + ']'

if len(argv) > 4:
    print(matrix)
if with_hash:
    print(rows_sums, cols_sums, m_hash_hex(matrix), seed)
else:
    print(rows_sums, cols_sums)
