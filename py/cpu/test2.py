import multiprocessing
from multiprocessing import Lock, Value


def deposit(balance: Value, lock: Lock):
    for i in range(500):
        with lock:
            balance.value += 1


def withdraw(balance: Value, lock: Lock):
    for i in range(500):
        with lock:
            balance.value -= 1


if __name__ == "__main__":
    lock = multiprocessing.Lock()
    x = multiprocessing.Condition()
    balance = multiprocessing.Value('i', 500)
    p1 = multiprocessing.Process(target=deposit, args=(balance, lock))
    p2 = multiprocessing.Process(target=withdraw, args=(balance, lock))
    p1.start()
    p2.start()
    p1.join()
    p2.join()
    print(balance.value)
