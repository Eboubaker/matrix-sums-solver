from numba import jit


@jit(target_backend='cuda', nopython=True)
def m_hash(matrix):
    """
    simple djb2 hashing
    :return: long
    """
    _hash = 5381
    for row in range(matrix.shape[0]):
        for col in range(matrix.shape[1]):
            _hash = ((_hash << 5) + _hash) + (1 if matrix[row, col] else 0) # hash * 33 + cell
    return _hash
