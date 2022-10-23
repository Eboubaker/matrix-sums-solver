from numba import jit


@jit(target_backend='cuda', nopython=True)
def m_hash(matrix):
    hs = ''
    for row in range(matrix.shape[0]):
        for col in range(matrix.shape[1]):
            hs = hs + ('1' if matrix[row, col] > 0 else '0')
    return hash(hs)
