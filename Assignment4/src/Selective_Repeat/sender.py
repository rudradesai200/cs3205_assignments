import argparse
import socket
import threading
import sys
from datetime import datetime, timedelta


def red(x): return "\033[0;31m" + str(x) + "\033[0m"
def green(x): return "\033[0;32m" + str(x) + "\033[0m"
def yellow(x): return "\033[0;33m" + str(x) + "\033[0m"
def blue(x): return "\033[0;34m" + str(x) + "\033[0m"


def setInterval(interval):
    def decorator(function):
        def wrapper(*args, **kwargs):
            stopped = threading.Event()

            def loop():  # executed in another thread
                while not stopped.wait(interval):  # until stopped
                    function(*args, **kwargs)

            t = threading.Thread(target=loop)
            t.daemon = True  # stop if the program exits
            t.start()
            return stopped
        return wrapper
    return decorator


class Sender:
    def __init__(self, args):
        self.args = args
        self.wndw_start = 1
        self.exit_signal = 0
        self.max_retransmit = 0
        self.rtt_total = 0.0
        self.packets_received = 0
        self.total_packs_sent = 0
        self.curr_buff_size = 0
        self.curr_seq_num = 1
        self.timeout = timedelta(milliseconds=300)
        self.min_timer = datetime.now() + timedelta(seconds=1000)

        self.acked = {}
        self.timers = {}
        self.order = []
        self.attempts = {}

        # self.lock_curr_buff_size = threading.Lock()
        # self.lock_attempts = threading.Lock()
        # self.lock_timers = threading.Lock()
        self.lock_send_packet = threading.Lock()
        # self.lock_min_timer = threading.Lock()

    def debug(self, status, msg):
        if((self.args.debug) and (status <= self.args.log_level)):
            if(status >= 3):
                print(
                    f"[{blue('LOG')} Sender {datetime.now().strftime('%S:%f')}] : {msg}")
            elif(status == 2):
                print(
                    f"[{yellow('INFO')} Sender {datetime.now().strftime('%S:%f')}] : {msg}")

            elif(status == 1):
                print(
                    f"[{green('DEBUG')} Sender {datetime.now().strftime('%S:%f')}] : {msg}")
            else:
                print(
                    f"[{red('ERROR')} Sender {datetime.now().strftime('%S:%f')}] : {msg}")

    def generate_packets(self):
        if(self.exit_signal):
            exit(-(self.exit_signal-1))

        self.curr_buff_size = max(
            self.args.buffer_size, self.curr_buff_size + self.args.pack_gen_rate)

    def receive_msg(self):
        # Start the udp server
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.debug(2, "Socket successfully created")
        except socket.error as err:
            self.debug(0, "socket creation failed with error %s" % (err))
            self.exit_signal = 2
            exit(-1)

        s.bind(('', self.args.send_port))
        self.debug(2, f"TCP server started on {self.args.send_port}")

        s.listen(16)
        while True:
            try:
                connection, client_address = s.accept()
            except socket.timeout:
                self.debug(
                    0, "Socket timed out. No messages received in sender.")
                self.exit_signal = True
                exit(-1)

            self.lock_send_packet.acquire()
            data = str(connection.recv(1024), encoding="utf-8")
            self.debug(2, f"Rcvd msg from {client_address} : {data}")

            if data.startswith('NAK'):
                split_str = data[3:].split(',')
                seq_num = int(split_str[0])
                seq_num1 = int(split_str[1][3:])
                attempts = int(split_str[2])
                self.recv_nak(seq_num)
                self.recv_ack(seq_num1, attempts)
            elif(data.startswith("ACK")):
                split_str = data[3:].split(',')
                seq_num = int(split_str[0])
                attempts = int(split_str[1])
                self.recv_ack(seq_num, attempts)
            else:
                self.debug(0, "Incorrect msg rcvd")
            self.lock_send_packet.release()

            if(self.max_retransmit >= 10):
                self.debug(
                    0, f"Retransmission limit reached : {self.max_retransmit}")
                self.exit_signal = 2
                exit(-1)

    def recv_nak(self, seq_num):
        self.debug(2, f"Rcvd {red('NAK')} for seq num {seq_num}")

        self.attempts[seq_num] += 1
        attempts = (self.attempts[seq_num])

        self.send_packet(seq_num, attempts)

    def recv_ack(self, seq_num, attempts):
        self.debug(2, f"Rcvd {green('ACK')} for seq num {seq_num}")
        if(len(self.timers) == 0):
            self.debug(
                0, f"Rcvd ACK for {seq_num} but no outstanding timer to clear.")
            self.exit_signal = 2
            self.lock_send_packet.release()
            exit(-1)

        rtt = (datetime.now() - self.timers[seq_num]).total_seconds()
        self.timers.pop(seq_num)

        self.rtt_total += rtt
        self.acked[seq_num] = True

        if (seq_num - self.args.window_size) in self.attempts.keys():
            self.attempts.pop(seq_num - self.args.window_size)

        self.max_retransmit = max(self.max_retransmit, attempts)
        form = "Seq %3d: Time %10s RTT: %5f Attempts: %s"
        self.debug(
            1, form % (seq_num, datetime.now().strftime('%S:%f'), rtt, red(attempts) if (attempts > 1) else green(attempts)))

        self.rtt_total += rtt
        self.packets_received += 1
        self.total_packs_sent += attempts

        self.min_timer = datetime.now() + timedelta(seconds=1000)
        while((len(self.order) > 0) and (self.order[0] in self.acked.keys())):
            self.min_seq_num = self.order[0] + 1
            self.debug(
                3, f"Min seq updated to {self.min_seq_num} in recv_ack_1")
            self.acked.pop(self.order[0])
            self.order.pop(0)
            self.wndw_start += 1

        if(len(self.order) == 0):
            self.min_seq_num = -1
            self.debug(
                3, f"Min seq updated to {self.min_seq_num} in recv_ack_2")
        else:

            self.min_timer = self.timers[self.min_seq_num]

        if(self.packets_received == self.args.max_packs):
            self.debug(2, "Sent all packets and rcvd all ACKS. Exiting ...")
            self.print_summary()
            self.exit_signal = 1
            self.lock_send_packet.release()
            exit(0)

        if(self.packets_received >= 10):
            self.timeout = timedelta(
                seconds=min(100*(self.rtt_total / self.packets_received), 0.3))

    def send_packet(self, seq_num, attempts):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        reciever_ip = socket.gethostbyname(self.args.recv_ip)
        receiver_addr = (reciever_ip, self.args.recv_port)

        connected = False
        while not connected:
            try:
                sock.connect(receiver_addr)
                connected = True
            except ConnectionRefusedError:
                continue

        pack = str(seq_num) + " " + str(attempts) + \
            " " + "A"*(self.args.max_pack_len)

        sock.sendall(str.encode(pack, encoding="utf-8"))
        sock.close()
        self.debug(2, f"Pack with seq_num {seq_num} sent")

    def process_timeout(self, seq_num):

        if(self.min_seq_num == -1):
            self.debug(0, "Min seq num set to -1 during timeout")
            self.exit_signal = 2
            self.lock_send_packet.release()
            exit(-1)

        self.min_timer = datetime.now()
        self.min_seq_num = seq_num

        self.debug(
            3, f"Min seq updated to {self.min_seq_num} in process_timeout")

        self.attempts[seq_num] += 1
        attempts = (self.attempts[seq_num])

        self.send_packet(seq_num, attempts)

    def send_next_packet(self):
        timer = datetime.now()

        self.timers[self.curr_seq_num] = timer

        self.order.append(self.curr_seq_num)

        self.attempts[self.curr_seq_num] = 1

        self.min_timer = min(self.min_timer, timer)
        if(self.min_timer == timer):
            self.min_seq_num = self.curr_seq_num
            self.debug(
                3, f"Min seq updated to {self.min_seq_num} in send_next_packet")

        self.send_packet(self.curr_seq_num, 1)

        self.curr_seq_num += 1
        self.curr_buff_size -= 1

    def send_packet_thread(self):
        while(True):
            if(self.exit_signal):
                exit(-(self.exit_signal - 1))

            self.lock_send_packet.acquire()
            if((datetime.now() - self.min_timer) > self.timeout):

                self.debug(
                    2, f"{red('Timeout')} for pack with seq num {self.min_seq_num}")
                self.process_timeout(self.min_seq_num)

            elif((self.curr_buff_size > 0) and (self.curr_seq_num <= self.args.max_packs) and (self.curr_seq_num < (self.wndw_start + self.args.window_size))):

                self.debug(
                    2, f"Sending next pack with seq_num {self.curr_seq_num}")
                self.send_next_packet()

            else:
                pass
            self.lock_send_packet.release()

    def print_summary(self):
        if(self.args.out != sys.stdout):
            self.args.out = open(self.args.out, "w+")

        self.args.out.write(
            f"Packet Generate Rate : {self.args.pack_gen_rate}\n")
        self.args.out.write(f"Packet Length : {self.args.max_pack_len}\n")
        self.args.out.write(
            f"Retransmission Ratio : {(self.total_packs_sent/self.args.max_packs)}\n")
        self.args.out.write(
            f"Average RTT : {self.rtt_total/ self.packets_received}\n")

        if(self.args.out != sys.stdout):
            self.args.out.close()

    def start_sender(self):
        self.debug(2, "Sender started")
        t1 = threading.Thread(target=self.receive_msg)
        t1.start()

        t2 = threading.Thread(target=self.send_packet_thread)
        t2.start()
        self.debug(2, "Started pack sender")

        @ setInterval(1)
        def fun1(): self.generate_packets()

        t3 = fun1()
        self.debug(2, "Started generating packets")


def parse_args():
    parser = argparse.ArgumentParser(
        description="Program to simulate Go Back N protocol.")
    parser.add_argument('-l', '--log_level', type=int,
                        default=1, help="Log level for debugging")
    parser.add_argument('-d', '--debug', type=bool,
                        default=False, help="Turn ON Debug Mode")
    parser.add_argument('-rip', '--recv_ip', type=str,
                        default="localhost", help="Receiver's hostname or IP address")
    parser.add_argument('-rp', '--recv_port', type=int,
                        default=10000, help="Receiver's Port Num")
    parser.add_argument('-n', '--seq_field_len', type=int,
                        default=8, help="Seq Num Field Length in bits")
    parser.add_argument('-L', '--max_pack_len', type=int,
                        default=10, help="Max pack length in bytes")
    parser.add_argument('-R', '--pack_gen_rate', type=int, default=20,
                        help="Pack generation rate in packets per second")
    parser.add_argument('-N', '--max_packs', type=int,
                        default=100,
                        help="Maximum packets to be sent")
    parser.add_argument('-W', '--window_size', type=int,
                        default=4, help="Window size for sending")
    parser.add_argument('-B', '--buffer_size', type=int,
                        default=64, help="Max buffer size")
    parser.add_argument('-sp', '--send_port', type=int, default=10001,
                        help="Sender's Port for receiving.")
    parser.add_argument('-o', '--out', type=str,
                        help="Outfile for printing sumamry. Defaults to sys.stdout")

    args = parser.parse_args()
    if not args.out:
        args.out = sys.stdout
    return args


if __name__ == "__main__":
    args = parse_args()
    sender = Sender(args)
    sender.start_sender()
