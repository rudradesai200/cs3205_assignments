import subprocess
import argparse
import os
import pandas as pd

df = pd.DataFrame()


def parse_summary(e):
    f = open("out.txt", "r")
    lines = f.readlines()
    print(lines)
    add_dict = {}
    add_dict['drop_prob'] = e
    add_dict['pack_gen_rate'] = lines[0].split(
        'Packet Generate Rate : ')[1][:-1]
    add_dict['pack_len'] = lines[1].split('Packet Length : ')[1][:-1]
    add_dict['retrans_ratio'] = lines[2].split(
        'Retransmission Ratio : ')[1][:-1]
    add_dict['avg_rtt'] = lines[3].split('Average RTT : ')[1][:-1]
    global df
    df = df.append(add_dict, ignore_index=True)
    f.close()


parser = argparse.ArgumentParser(
    description="Program to run multiple routers for Assignment 3 simulation")
parser.add_argument('-rip', '--recv_ip', type=str,
                    default="127.0.0.1", help="Receiver's hostname or IP address")
parser.add_argument('-rp', '--recv_port', type=int,
                    default=10000, help="Receiver's Port Number")
parser.add_argument('-N', '--max_packs', type=int,
                    default=300,
                    help="Maximum packets for communication")
parser.add_argument('-W', '--window_size', type=int,
                    default=4, help="Window size for sending")
parser.add_argument('-B', '--buffer_size', type=int,
                    default=64, help="Max buffer size")
parser.add_argument('-sp', '--send_port', type=int, default=10001,
                    help="Sender's Port for receiving.")
parser.add_argument('-sip', '--sender_ip', default="127.0.0.1", type=str,
                    help="IP address or hostname of the sender")
args = parser.parse_args()


for pack_gen_rate in [20, 60, 180, 300]:
    for pack_len in [256, 512, 1024]:
        for drop_prob in [10**-3, 10**-2, 0.1, 0.3]:
            print("pack_gen_rate", pack_gen_rate, "pack_len",
                  pack_len, "drop_prob", drop_prob)
            p1 = subprocess.Popen(['python', 'receiver.py', '-sip', args.sender_ip, '-sp', str(args.send_port),
                                   '-rp', str(args.recv_port), '-n', str(args.max_packs), '-e', str(drop_prob), '-d', str('True'), '-l', str(1)])
            p2 = subprocess.Popen(['python', 'sender.py', '-rip', args.recv_ip, '-sp', str(args.send_port),
                                   '-rp', str(args.recv_port), '-N', str(args.max_packs), '-B', str(args.buffer_size), '-W', str(args.window_size), '-R', str(pack_gen_rate), '-L', str(pack_len), '-d', str('True'), '-l', str(1), '-o', str('out.txt')])

            retcode1 = p1.wait()
            retcode2 = p2.wait()
            if(retcode1 != 0) or (retcode2 != 0):
                exit(-1)

            parse_summary(drop_prob)


df.to_csv('../../results/GBN_results.csv', index=False)
os.remove('out.txt')
