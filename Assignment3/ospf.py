import argparse
import socket
from random import randint
from numpy import zeros
from sys import maxsize
import threading
from datetime import datetime
import os
from graphviz import Graph

local_IP = "127.0.0.1"
port_Prefix = 10000
BUFFER_SIZE = 1024


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


class Router:
    def __init__(self, id, infile, outfile, hello, lsa, spf, log_level):
        self.id = id
        self.infile = infile
        self.outfile = outfile
        self.hello = hello
        self.lsa = lsa
        self.spf = spf
        self.log_level = log_level
        self.neighbours = {}
        self.seq_nums = {}
        self.seq_no = 0
        self.lock = threading.Lock()
        self.parse_infile()

    def parse_infile(self):
        # Read infile and get lines
        infile = open(self.infile, "r")
        lines = infile.readlines()

        # Split string and set values
        t1, t2 = lines[0].split(",")
        self.routers = int(t1)
        self.edges = int(t2)
        with self.lock:
            self.graph = zeros((self.routers, self.routers))

        # Read lines and set neighbours
        for idx in range(1, self.edges+1):
            i, j, mini, maxi = lines[idx].split(",")
            if(int(i) == self.id):
                self.neighbours[int(j)] = (int(mini), int(maxi))
            elif(int(j) == self.id):
                self.neighbours[int(i)] = (int(mini), int(maxi))
            else:
                pass

    def send_msg(self, id, msg):
        UDPClientSocket = socket.socket(
            family=socket.AF_INET, type=socket.SOCK_DGRAM)
        UDPClientSocket.sendto(str.encode(
            f"{self.id},"+msg), (local_IP, id+port_Prefix))
        self.debug(1, f"Message sent on {id} : {msg}")

    def receive_msg(self):
        """
        Starts the UDP server at port (port_Prefix + id) and waits to receive packets.
        """
        # Start the udp server
        UDPServerSocket = socket.socket(
            family=socket.AF_INET, type=socket.SOCK_DGRAM)
        # Bind the server with port (port_Prefix + id)
        UDPServerSocket.bind((local_IP, port_Prefix + self.id))
        self.debug(2, f"UDP server started on {port_Prefix+self.id}")

        # Wait for receiving packets
        while True:
            bytesAddressPair = UDPServerSocket.recvfrom(BUFFER_SIZE)
            message = str(bytesAddressPair[0], "utf-8")
            first_comma = message.find(',')
            rcv_id = int(message[:first_comma])
            self.debug(1, f"Received message from {rcv_id} : {message}")
            message = message[first_comma+1:]

            # Check the message and process accordingly
            if(message[:message.find(',')] == "HELLO"):
                self.send_hello_reply(message, rcv_id)
            elif(message.startswith("HELLOREPLY")):
                self.accept_hello_reply(message, rcv_id)
            elif(message.startswith("LSA")):
                self.receive_lsa(message, rcv_id)
            else:
                raise ValueError(
                    f"Incorrect msg received : {message} from {rcv_id}")

    def send_hello(self):
        for id in self.neighbours:
            self.send_msg(id, f"HELLO,{self.id}")

    def send_hello_reply(self, msg, rcv_router_id):
        rcv_id = int(msg.split(',')[1])
        if(rcv_id != rcv_router_id):
            raise KeyError(
                f"Receiver id {rcv_router_id} != id in msg {rcv_id}")
        new_cost = randint(
            self.neighbours[rcv_id][0], self.neighbours[rcv_id][1])
        self.send_msg(rcv_id, f"HELLOREPLY,{self.id},{rcv_id},{new_cost}")
        with self.lock:
            self.graph[self.id, rcv_id] = new_cost
        self.debug(2, f"HELLO ACK to {rcv_id} with cost {new_cost}")

    def accept_hello_reply(self, msg, rcv_router_id):
        tokens = msg.split(',')
        j, i, linkij = int(tokens[1]), int(tokens[2]), int(tokens[3])
        if(j != rcv_router_id):
            raise KeyError(f"Receiver id {rcv_router_id} != id in msg {j}")
        if(i != self.id):
            raise KeyError(
                f"Incorrect message received. Directed to {i} received by {self.id}")
        with self.lock:
            self.graph[i][j] = int(linkij)
        self.debug(2, f"HELLO ACK received from {j} with cost {linkij}")

    def send_lsa(self):
        self.seq_no += 1
        msg = f"LSA,{self.id},{self.seq_no},"
        neighs = 0
        lsa_suffix = ""
        for r in range(self.routers):
            with self.lock:
                if(self.graph[self.id][r] != maxsize):
                    lsa_suffix += f",{r},{int(self.graph[self.id][r])}"
                    neighs += 1

        msg += f"{neighs}{lsa_suffix}"

        for n in range(self.routers):
            self.send_msg(n, msg)
            self.debug(2, f"LSA sent to {n}")

    def receive_lsa(self, msg, rcv_router_id):
        tokens = msg.split(',')
        srcid = int(tokens[1])
        seq_num = int(tokens[2])
        entries = int(tokens[3])
        self.debug(2, f"LSA received from {srcid}")

        if srcid not in self.seq_nums.keys():
            self.seq_nums[srcid] = 0
        if(seq_num <= self.seq_nums[srcid]):
            return

        self.seq_nums[srcid] = seq_num
        for i in range(entries):
            neigh = int(tokens[4+2*i])
            cost = int(tokens[5+2*i])
            with self.lock:
                self.graph[srcid][neigh] = cost

        # Forward this message to all the neighbours except the rcv_router
        for n in self.neighbours:
            if(n != rcv_router_id):
                self.send_msg(n, msg)
                self.debug(2, f"LSA forwarded to {n}")

    def debug(self, status, msg):
        if(status <= self.log_level):
            if(status >= 2):
                print(
                    f"[\033[0;32mDEBUG\033[0m Router-{self.id} {datetime.now().strftime('%H:%M:%S')}] : {msg}")
            elif(status == 1):
                print(
                    f"[\033[0;33mINFO\033[0m Router-{self.id} {datetime.now().strftime('%H:%M:%S')}] : {msg}")
            else:
                print(
                    f"[\033[0;34mMSG\033[0m Router-{self.id} {datetime.now().strftime('%H:%M:%S')}] : {msg}")

    def minDistance(self, dist, sptSet):

        # Initilaize minimum distance for next node
        mini = maxsize
        min_index = -1
        # Search not nearest vertex not in the
        # shortest path tree
        for v in range(self.routers):
            if dist[v] < mini and sptSet[v] == False:
                mini = dist[v]
                min_index = v

        return min_index

    def getPath(self, j):
        if self.parent[j] == -1:
            return str(j)
        return self.getPath(self.parent[j]) + "->" + str(j)

    def dijkstra(self):
        dist = [maxsize] * self.routers
        dist[self.id] = 0
        sptSet = [False] * self.routers
        self.parent = [-1] * self.routers

        for _ in range(self.routers):
            u = self.minDistance(dist, sptSet)
            sptSet[u] = True
            for v in range(self.routers):
                with self.lock:
                    if ((self.graph[u][v] > 0) and
                        (sptSet[v] == False) and
                            (dist[v] > dist[u] + self.graph[u][v])):
                        dist[v] = dist[u] + self.graph[u][v]
                        self.parent[v] = u

        self.dist = dist

    def plotGraph(self):
        def get_name(a, b): return f"start_{b}" if (a == b) else f"router_{a}"

        g = Graph(self.outfile.replace(".txt", ""), filename=self.outfile.replace(".txt", ".gv"),
                  engine='sfdp', strict=True)
        g.attr('node', style='filled', color="lightblue")

        with g.subgraph(name='shortest_path') as c:
            for i in range(self.routers):
                if(self.parent[i] != -1):
                    c.edge(get_name(self.parent[i], self.id), get_name(
                        i, self.id), label=f"{int(self.graph[self.parent[i]][i])}", color="green")

        for i in range(self.routers):
            for j in range(i+1, self.routers):
                if self.graph[i][j] != 0:
                    g.edge(get_name(i, self.id), get_name(
                        j, self.id), label=f"{int(self.graph[i][j])}")

        g.render(filename=self.outfile.replace(".txt", ""), format="png")

    def write_outfile(self):
        outfile = open(self.outfile, "a")
        outfile.write(f"{self.id},{datetime.now().strftime('%H:%M:%S')}\n")
        for r in range(self.routers):
            if(r != self.id):
                outfile.write(f"{r},{self.getPath(r)},{int(self.dist[r])}\n")
        self.debug(2, f"Outfile updated by {self.id}")

    def spf_thread_fn(self):
        self.dijkstra()
        self.debug(1, "Shortest paths updated.")
        self.write_outfile()
        self.debug(3, self.graph)
        self.plotGraph()

    def start_router(self):
        self.debug(2, "Router started")
        t1 = threading.Thread(target=self.receive_msg)
        t1.start()
        self.debug(2, "Server started")

        @setInterval(self.hello)
        def fun1(): self.send_hello()

        @setInterval(self.lsa)
        def fun2(): self.send_lsa()

        @setInterval(self.spf)
        def fun3(): self.spf_thread_fn()

        t2 = fun1()
        self.debug(2, "Hello thread started")

        t3 = fun2()
        self.debug(2, "LSA thread started")

        t4 = fun3()
        self.debug(2, "SPF thread started")


def parse_args():
    parser = argparse.ArgumentParser(
        description="Program to simulate a router using OSPF routing algorithm.")
    parser.add_argument('-i', '--id', type=int,
                        help="id of the router", required=True)
    parser.add_argument('-o', '--outfile', type=str,
                        help="outfile for the graph")
    parser.add_argument('-f', '--infile', type=str,
                        help="infile for the graph. default = infile.txt", default="infile.txt")
    parser.add_argument('-H', '--hello_interval', type=int,
                        help="HELLO interval for OSPF. default = 3", default=3)
    parser.add_argument('-a', '--lsa_interval', type=int,
                        help="LSA interval for OSPF. default = 4", default=4)
    parser.add_argument('-s', '--spf_interval', type=int,
                        help="SPF interval for OSPF. default = 10", default=10)
    parser.add_argument('-l', '--log_level', type=int,
                        help="Logging level for debugging messages. default = 1", default=1)

    args = parser.parse_args()
    if(args.outfile == None):
        os.makedirs("outputs", exist_ok=True)
        args.outfile = f"outputs/outfile-{args.id}.txt"

    return args


if __name__ == "__main__":
    args = parse_args()

    r = Router(args.id, args.infile, args.outfile,
               args.hello_interval, args.lsa_interval, args.spf_interval, args.log_level)
    r.start_router()
