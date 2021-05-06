import argparse
import socket
import threading
import time
from datetime import datetime

from numpy.random import binomial, seed


def red(x): return "\033[0;31m" + str(x) + "\033[0m"
def green(x): return "\033[0;32m" + str(x) + "\033[0m"
def yellow(x): return "\033[0;33m" + str(x) + "\033[0m"
def blue(x): return "\033[0;34m" + str(x) + "\033[0m"


class Receiver:
    def __init__(self, args):
        self.args = args
        self.last_ack = 0
        self.exit_signal = False

    def debug(self, status, msg):
        if((self.args.debug) and (status <= self.args.log_level)):
            if(status >= 2):
                print(
                    f"[{yellow('INFO')} Receiver {datetime.now().strftime('%S:%f')}] : {msg}")

            elif(status == 1):
                print(
                    f"[{green('DEBUG')} Receiver {datetime.now().strftime('%S:%f')}] : {msg}")
            else:
                print(
                    f"[{red('ERROR')} Receiver {datetime.now().strftime('%S:%f')}] : {msg}")

    def send_ack(self, seq_num):
        if(seq_num == self.last_ack + 1):
            accept = binomial(1, 1 - self.args.pack_err)
            form = "Seq %3d: Time %10s Packet dropped: %s"
            self.debug(
                1, form % (seq_num, datetime.now().strftime('%S:%f'),
                           red('True') if(accept == 0) else green('False')))

            if(accept):
                sock = socket.socket(
                    family=socket.AF_INET, type=socket.SOCK_STREAM)
                sender_ip = socket.gethostbyname(self.args.sender_ip)
                sender_address = (sender_ip, self.args.sender_port)
                try:
                    sock.connect(sender_address)
                except:
                    self.debug(0, "Sender server closed. Exiting ...")
                    self.exit_signal = True
                    exit(-1)

                packet = f"ACK {seq_num}"

                sock.sendall(str.encode(packet, encoding="utf-8"))
                sock.close()
                self.debug(
                    2, f"Sent {blue('ACK')} for packet with sequence enumber {seq_num}")
                self.last_ack += 1
            else:
                self.debug(
                    2, f"{red('Dropped')} packet with seq num {seq_num}")
        else:
            self.debug(
                2, f"Received {yellow('out-of-order')} packet with seq num {seq_num} expecting {self.last_ack + 1}")

    def receive_msg(self):
        """
        Starts the UDP server at port (args.sp) and waits to receive packets.
        """
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.debug(2, "Socket successfully created")
        except socket.error as err:
            self.debug(0, "socket creation failed with error %s" % (err))
            self.exit_signal = 2
            exit(-1)

        s.bind(('', self.args.recv_port))
        self.debug(2, f"TCP server started on {self.args.recv_port}")

        s.listen(16)

        # Wait for receiving packets
        while True:
            try:
                connection, client_address = s.accept()
            except socket.timeout:
                self.debug(0, "Socket timed out. No messages received.")
                self.exit_signal = True
                exit(-1)

            data = str(connection.recv(1024), encoding="utf-8")
            self.debug(2, f"Received message from {client_address} : {data}")

            seq_num = data.split(' ')[0]
            if(seq_num.isnumeric()):
                self.send_ack(int(seq_num))
            else:
                self.debug(0, "Incorrect message received. Ignoring...")
                continue

            if(self.last_ack >= self.args.max_packs):
                self.debug(
                    2, "Received all segments and acknowledged. Exiting...")
                time.sleep(0.1)
                self.exit_signal = True
                exit(0)

    def start_receiver(self):
        self.debug(2, "Receiver started")
        while(True):
            self.receive_msg()


def parse_args():
    parser = argparse.ArgumentParser(
        description="Program to simulate Go Back N protocol.")
    parser.add_argument('-l', '--log_level', type=int,
                        default=1, help="Log level for debugging")
    parser.add_argument('-d', '--debug', type=bool,
                        default=False, help="Turn ON Debug Mode")
    parser.add_argument('-rp', '--recv_port', type=int,
                        default=10000, help="Receiver's Port Number")
    parser.add_argument('-sip', '--sender_ip', default="127.0.0.1", type=str,
                        help="IP address or hostname of the sender")
    parser.add_argument('-sp', '--sender_port', default=10001, type=int,
                        help="Sender's port num")
    parser.add_argument('-n', '--max_packs', type=int,
                        default=100,
                        help="Maximum packets to be sent")
    parser.add_argument('-e', '--pack_err', type=float,
                        default=0.1, help="Probability of packet being dropped")
    args = parser.parse_args()
    return args


if __name__ == "__main__":
    seed(19)
    args = parse_args()
    receiver = Receiver(args)
    receiver.start_receiver()
