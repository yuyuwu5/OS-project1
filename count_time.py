INPUT = "time.txt"
total = 0
with open(INPUT) as f:
    for l in f:
        a = l.split()
        start = a[3]
        end =  a[4]
        t = float(end)-float(start)
        total += t
        #print(start, end)
        #print(t)
print("avg: %f"%(total/5000))
