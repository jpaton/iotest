from matplotlib import pyplot as plt
import numpy as np
import sys, csv

reader = csv.DictReader(open(sys.argv[1], 'r'), fieldnames = ['block', 'time'])

raw_data = sorted([(int(row['block']), int(row['time'])) for row  in reader])
data = list()

block = raw_data[0][0]
total = 0.0
N = 0.0
for item in raw_data:
    if item[0] != block:
        data.append((block, total / N))
        block = item[0]
        total = item[1]
        N = 1.0
    else:
        total += item[1]
        N += 1
data.append((block, total / N))

if len(sys.argv) > 2:
	maxBlock = int(sys.argv[2])
else:
	maxBlock = len(data)

subset = zip(*data[:maxBlock])[1]
print subset[:20]
mean = np.mean(subset)
stddev = np.std(subset)
print "N: %d" % len(subset)
print "Mean: %f" % mean
print "Std. dev.: %f" % stddev
print "Std. dev. compared to mean: %f" % (stddev / mean)
plt.plot(subset)
plt.show()
