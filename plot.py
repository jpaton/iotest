from matplotlib import pyplot as plt
import numpy as np
import sys, csv

reader = csv.DictReader(open(sys.argv[1], 'r'), fieldnames = ['block', 'time'])

data = sorted([(int(row['block']), int(row['time'])) for row  in reader])

if len(sys.argv) > 2:
	maxBlock = int(sys.argv[2])
else:
	maxBlock = len(data)

subset = list(zip(*data[:maxBlock])[1])
print subset[:20]
mean = np.mean(subset)
stddev = np.std(subset)
print "N: %d" % len(subset)
print "Mean: %f" % mean
print "Std. dev.: %f" % stddev
print "Std. dev. compared to mean: %f" % (stddev / mean)
print "Auto correlation: " + str(np.correlate(subset, subset, mode='same'))
plt.plot(subset)
plt.show()
