import numpy
import collections


def map(x, x_min, x_max, is_bound=True):
    x = numpy.asarray(x)                        # Convert input to a numpy array
    output = (x - x_min) / (x_max - x_min)      # normalize to the range: min and max
    if is_bound:                                # decide if the output bound to the range
        output[x >= x_max] = 1.0
        output[x <= x_min] = 0.0
    
    # convert the result back to its original type: float or list 
    return output.tolist() if isinstance(x, numpy.ndarray) else float(output)


def nearest(array, val):
    # get the index of array that has value closest to val
    array = numpy.asarray(array)
    return (numpy.abs(array - val)).argmin()
    
    
def get_fourier(array, rate):
    # get the fourier transform plot (x-y axes)
    n = len(array)
    fft = numpy.fft.fft(numpy.asarray(array))[1:n//2]
    freq = numpy.fft.fftfreq(n, 1/rate)[1:n//2]
    energy = numpy.abs(fft) ** 2
    
    return freq, energy


def sigmoid(x):
    return 1. / (1 + numpy.exp(-x))


def sigmoid_derivative(values):
    return values * (1 - values)


def tanh_derivative(values):
    return 1. - values ** 2


def rand_arr(a, b, *args):
    # create uniform random array with values in [a, b) and shaped in args
    return numpy.random.rand(*args) * (b - a) + a


class Quaternion:
    def __init__(self, w, x, y, z):
        self.w = w
        self.x = x
        self.y = y
        self.z = z
        self.vector = numpy.array([w, x, y, z])

    def __mul__(self, quat):
        w = self.w * quat.w - self.x * quat.x - self.y * quat.y - self.z * quat.z
        x = self.w * quat.x + self.x * quat.w + self.y * quat.z - self.z * quat.y
        y = self.w * quat.y - self.x * quat.z + self.y * quat.w + self.z * quat.x
        z = self.w * quat.z + self.x * quat.y - self.y * quat.x + self.z * quat.w
        return Quaternion(w, x, y, z)
    
    def inv(self):
        w = self.w
        x = -self.x
        y = -self.y
        z = -self.z
        return Quaternion(w, x, y, z)
    
    def angle(self, quat):
        rel = quat.inv() * self
        rel_angle =  2 * numpy.arctan2(numpy.linalg.norm(rel.vector[1:4]), rel.vector[0])
        return rel_angle


class Butterworth:
    FILTER_BUFFER_SIZE = 3
    
    def __init__(self):
        self.buffer_in = collections.deque(numpy.zeros(self.FILTER_BUFFER_SIZE), maxlen=self.FILTER_BUFFER_SIZE)
        self.buffer_out = collections.deque(numpy.zeros(self.FILTER_BUFFER_SIZE), maxlen=self.FILTER_BUFFER_SIZE)
        self.a = numpy.zeros(self.FILTER_BUFFER_SIZE)
        self.b = numpy.zeros(self.FILTER_BUFFER_SIZE)
        self.enable = True

    def butter(self, cutoff, sampling):
        if cutoff is None:
            self.enable = False
            return None, None
            
        # polynomial coefficient of 2nd order Butterworth filter
        ita = 1.0 / numpy.tan(numpy.pi * cutoff / sampling)
        q = numpy.sqrt(2.0)
        self.b[0] = 1.0 / (1.0 + q * ita + ita * ita)
        self.b[1] = self.b[0] * 2
        self.b[2] = self.b[0]
        self.a[1] = 2.0 * (ita * ita - 1.0) * self.b[0]
        self.a[2] = -(1.0 - q * ita + ita * ita) * self.b[0]
        
        return self.a, self.b
    
    def feed(self, data_in):
        if self.enable:
            # register and shift data into buffer array
            self.buffer_in.appendleft(data_in)
            self.buffer_out.appendleft(0.0)
            # compute filtered data using 2nd order Butterworth filter
            for i in range(3):
                self.buffer_out[0] += self.b[i] * self.buffer_in[i]
                self.buffer_out[0] += self.a[i] * self.buffer_out[i]
        
            return self.buffer_out[0]
        
        else:
            return data_in
    