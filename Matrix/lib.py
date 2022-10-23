from hashlib import md5


def m_hash(matrix):
    return md5(matrix).hexdigest()
