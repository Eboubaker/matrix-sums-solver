from hashlib import md5


def m_hash_hex(matrix):
    return md5(matrix).hexdigest()

def m_hash_digest(matrix):
    return md5(matrix).digest()
