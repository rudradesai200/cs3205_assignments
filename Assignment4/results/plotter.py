import matplotlib.pyplot as plt
import pandas as pd
import cv2


GBN = pd.read_csv("GBN_results.csv")
GBN = GBN.rename(columns={"avg_rtt": "GBN Avg RTT",
                          "retrans_ratio": "GBN Retransmission Ratio"})
SR = pd.read_csv("SR_results.csv")
SR = SR.rename(columns={"avg_rtt": "SR Avg RTT",
                        "retrans_ratio": "SR Retransmission Ratio"})

df = pd.concat(
    [GBN, SR[["SR Avg RTT", "SR Retransmission Ratio"]]], axis=1, join="inner")

df = df.rename(columns={"drop_prob": "Drop Probability",
                        "pack_gen_rate": "Packet Generate Rate", "pack_len": "Max Packet Length"})

df.groupby("Drop Probability").mean()[
    ['GBN Avg RTT', 'SR Avg RTT']].plot(legend=True)
plt.savefig('../plots/avg_rtt_vs_drop_prob.png')

df.groupby("Packet Generate Rate").mean()[
    ['GBN Avg RTT', 'SR Avg RTT']].plot(legend=True)
plt.savefig('../plots/avg_rtt_vs_pack_gen_rate.png')

df.groupby("Max Packet Length").mean()[
    ['GBN Avg RTT', 'SR Avg RTT']].plot(legend=True)
plt.savefig('../plots/avg_rtt_vs_pack_len.png')

df.groupby("Drop Probability").mean()[
    ['GBN Retransmission Ratio', 'SR Retransmission Ratio']].plot(legend=True)
plt.savefig('../plots/retrans_ratio_vs_drop_prob.png')

df.groupby("Packet Generate Rate").mean()[
    ['GBN Retransmission Ratio', 'SR Retransmission Ratio']].plot(legend=True)
plt.savefig('../plots/retrans_ratio_vs_pack_gen_rate.png')

df.groupby("Max Packet Length").mean()[
    ['GBN Retransmission Ratio', 'SR Retransmission Ratio']].plot(legend=True)
plt.savefig('../plots/retrans_ratio_vs_pack_len.png')


img1 = cv2.imread('../plots/avg_rtt_vs_drop_prob.png')
img2 = cv2.imread('../plots/avg_rtt_vs_pack_gen_rate.png')
img3 = cv2.imread('../plots/avg_rtt_vs_pack_len.png')

fig = plt.figure(figsize=(16, 8))
fig.add_subplot(2, 3, 1)
plt.axis("off")
plt.imshow(img1)
fig.add_subplot(2, 3, 2)
plt.axis("off")
plt.imshow(img2)
fig.add_subplot(2, 3, 3)
plt.axis("off")
plt.imshow(img3)

# plt.savefig('../plots/avg_rtt.png')

img1 = cv2.imread('../plots/retrans_ratio_vs_drop_prob.png')
img2 = cv2.imread('../plots/retrans_ratio_vs_pack_gen_rate.png')
img3 = cv2.imread('../plots/retrans_ratio_vs_pack_len.png')

# fig = plt.figure(figsize=(35, 7))
fig.add_subplot(2, 3, 4)
plt.axis("off")
plt.imshow(img1)
fig.add_subplot(2, 3, 5)
plt.axis("off")
plt.imshow(img2)
fig.add_subplot(2, 3, 6)
plt.axis("off")
plt.imshow(img3)

plt.savefig('../plots/collage.png')
