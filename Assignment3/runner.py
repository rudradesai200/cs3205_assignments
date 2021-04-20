import subprocess
import argparse
from datetime import datetime, timedelta
import os


parser = argparse.ArgumentParser(
    description="Program to run multiple routers for Assignment 3 simulation")
parser.add_argument("-n", "--nrouters",
                    help="Number of routers.", required=True, type=int)
parser.add_argument(
    "-t", "--runtime", help="Number of seconds to wait before close the router. default = 20", default=20, type=int)
parser.add_argument('-f', '--infile', type=str,
                    help="infile for the graph. default = infile.txt", default="infile.txt")
parser.add_argument('-o', '--output_dir', type=str,
                    help="output directory for the graph. default = outputs", default="outputs")
parser.add_argument('-H', '--hello_interval', type=int,
                    help="HELLO interval for OSPF. default = 3", default=3)
parser.add_argument('-a', '--lsa_interval', type=int,
                    help="LSA interval for OSPF. default = 4", default=4)
parser.add_argument('-s', '--spf_interval', type=int,
                    help="SPF interval for OSPF. default = 10", default=10)
parser.add_argument('-l', '--log_level', type=int,
                    help="Logging level for debugging messages. default = 1", default=1)

args = parser.parse_args()

os.makedirs(args.output_dir, exist_ok=True)
pids = []
for i in range(args.nrouters):
    p = subprocess.Popen(
        ['python', 'ospf.py', '-i', f"{i}", "-o", f"{args.output_dir}/output{i+1}.txt", "-l", str(args.log_level), "-H", str(args.hello_interval), "-a", str(args.lsa_interval), "-s", str(args.spf_interval), "-f", args.infile])
    pids.append(str(p.pid))

start = datetime.now()
while True:
    end = datetime.now()
    if(end-start >= timedelta(seconds=args.runtime)):
        break

k = ["kill", "-9"]
k.extend(pids)
subprocess.Popen(k)
