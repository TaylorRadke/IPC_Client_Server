import sys

def trial(num):
    a = []
    f = 2

    while num > 1:
        if num % f == 0:
            print(f)
            a.append(f)
            num /= f
        else:
            f += 1
    print(a)

trial(int(sys.argv[1]))