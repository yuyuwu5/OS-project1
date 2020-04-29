from argparse import ArgumentParser
parser = ArgumentParser()
parser.add_argument("--file", type=str)
parser.add_argument("--perfect", type=str)
parser.add_argument("--unit", type=float, default=0.0018)
args = parser.parse_args()
process = 0
up_start = 0
ans={}

with open(args.perfect) as f:
    for l in f:
        data = l.split()
        if data[0] == "Create":
            pid =  data[3]
            start = [float(data[5])]
            ans[pid] = start
        elif data[1] == "end":
            pid = data[0]
            ans[pid].append(float(data[3]))
        elif data[0] == "Real":
            up_start = float(data[2])

with open(args.file) as f:
    s = []
    for i, l in enumerate(f):
        data = l.split()
        if data[2]=='0':continue
        s.append( float(data[3]))
    real_start = min(s)

with open(args.file) as f:
    for i, l in enumerate(f):
        data = l.split()
        if data[2]=='0':continue
        pid = data[2]
        start = float(data[3])
        end = float(data[4])
        st = (start-real_start)/args.unit+up_start
        ed = (end-real_start)/args.unit+up_start
        print("perfect {:^8}, start {:^8f}, end_time {:^8f}".format(pid, ans[pid][0], ans[pid][1]))
        print("real_exp {:^7}, start {:^8f}, end_time {:^8f}".format("   ", st, ed))
        print()

