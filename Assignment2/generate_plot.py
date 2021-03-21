import numpy as np
import matplotlib.pyplot as plt
import re


# def get_x_and_cws():
#     x = []
#     cws = []
#     thresh = []
#     f = open("cws.txt", "r")
#     while True:
#         st = f.readline()
#         if not st:
#             break
#         m = re.search(r'Update : (\d+) CWS : (\d+) Thresh : (\d+)', st)
#         x.append(int(m.group(1)))
#         cws.append(int(m.group(2)))
#         thresh.append(int(m.group(3)))
#     f.close()
#     return x, cws, thresh


# x, cws, thresh = get_x_and_cws()
# x = [i for i in range(1,)]
n = 0
cws = []
f = open("output.txt", "r")
while True:
    st = f.readline()
    if not st:
        break
    cws.append(float(st))
    n += 1

x = [i for i in range(1, n+1)]
plt.plot(x, cws)
# plt.plot(x, thresh)
plt.show()
