import re
from sys import argv

import numpy as np

from lib import m_hash_hex
# matrix rows cols => hash
print(m_hash_hex(
    np.array(
        [
            np.fromstring(x[0], dtype=int, sep=',')
            for x
            in re.findall(r"\[((\d+,)+?\d+)]", argv[1])
            if len(x) > 1
        ]
    ).reshape(int(argv[2]), int(argv[3]))
))
