import subprocess
import matplotlib.pyplot as plt


def save_plot(fname, title):
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
    plt.plot(x, cws, marker='.', markersize=3, linewidth=1)
    plt.xlabel("Update idx")
    plt.ylabel("CWS Value")
    plt.title(title)
    plt.savefig(fname)
    plt.clf()


j = 0
for i in [1,  4]:
    for m in [1.0, 1.5]:
        for n in [0.5, 1]:
            for f in [0.1, 0.3]:
                for s in [0.01, 0.0001]:
                    subprocess.run(["./simulation", "-o", "output.txt", "-T",
                                    "1000", "-i", f"{i}", "-m", f"{m}", "-n", f"{n}", "-f", f"{f}", "-s", f"{s}"])
                    save_plot(f"plots/plt_{j}.png",
                              f"Ki={i} Km={m} Kn={n} Kf={f} Ps={s}")
                    j += 1
