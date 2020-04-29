import pandas as pd
#a = pd.read_csv("ExpResult.txt")#, delimiter=',')
with open("ExpResult.txt") as f:
    err = p = 0
    for i, l in enumerate(f):
        d = l.split()
        ll = len(d)
        if ll==0:
            continue
        if ll==1:
            if i!=0:
                print("%s avg err %f"%(name, err/p))
            err = 0
            p = 0
            name = d[0]
        if ll==6:
            now = float(d[-1])
            err += abs(now-pre)
            p += 1
        if ll==7:
            pre = float(d[-1])
print("%s avg err %f"%(name, err/p))
