from Toolbox import of_CSV
from Toolbox import of_Math
from matplotlib import pyplot
import numpy


# path
PATH = 'SessionData_co_not.csv'
PATH = 'SessionData_co_yes.csv'

# header
header = ['index', 'session', 'time', 'fd', 'ed']
header += ['fd_th', 'ed_th', 'move']
header += ['fd_0', 'ed_0', 'fd_mvc', 'ed_mvc', 'dir']

# data
# data1 = of_CSV.Helper(PATH, ','.join(header))
# data1.read_raw_data()

# fd = of_Math.map(data1.array['fd'], data1.array['fd_0'], data1.array['fd_mvc'])
# ed = of_Math.map(data1.array['ed'], data1.array['ed_0'], data1.array['ed_mvc'])

with open(PATH, mode='r', encoding='utf-8-sig') as f:
    data = numpy.genfromtxt(f, names=','.join(header), delimiter=',')

fd = of_Math.map(data['fd'], data['fd_0'], data['fd_mvc'])
ed = of_Math.map(data['ed'], data['ed_0'], data['ed_mvc'])

# co-contraction
cii_min = numpy.sum(numpy.minimum(fd, ed))
cii_sum = numpy.sum(fd) + numpy.sum(ed)
cii = 2.0 * cii_min / cii_sum
    

# plot
pyplot.figure('Test')

pyplot.plot(data['time'], fd, color='b')
pyplot.plot(data['time'], ed, color='r')

pyplot.text(x=0, y=1, s=cii)

pyplot.grid(True)

pyplot.show()
