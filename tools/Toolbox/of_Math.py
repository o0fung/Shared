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
    def __init__(self, w=None, x=None, y=None, z=None):
        if w is None or x is None or y is None or z is None:
            return
        
        self.set_vector(w, x, y, z)

    def __mul__(self, quat):
        w = self.w * quat.w - self.x * quat.x - self.y * quat.y - self.z * quat.z
        x = self.w * quat.x + self.x * quat.w + self.y * quat.z - self.z * quat.y
        y = self.w * quat.y - self.x * quat.z + self.y * quat.w + self.z * quat.x
        z = self.w * quat.z + self.x * quat.y - self.y * quat.x + self.z * quat.w
        return Quaternion(w, x, y, z)
    
    def set_vector(self, w, x, y, z):
        self.w = w
        self.x = x
        self.y = y
        self.z = z
        self.vector = numpy.array([w, x, y, z])
        
    def magnitude(self):
        # Get magnitude of the quaternion
        # Equivalent to squared norm: i.e. q* dot q
        return numpy.sqrt(self.w ** 2 + self.x ** 2 + self.y ** 2 + self.z ** 2)
    
    def norm(self):
        # Get unit quaternion
        mag = self.magnitude()
        return Quaternion(self.w/mag, self.x/mag, self.y/mag, self.z/mag)
    
    def inv(self):
        # Inverse of the quaternion
        # For unit quaternion, inverse is equal to conjugate
        w = self.w
        x = -self.x
        y = -self.y
        z = -self.z
        return Quaternion(w, x, y, z)
    
    def dot(self, quat):
        # Dot product of two quaternion
        return self.w * quat.w + self.x * quat.x + self.y * quat.y + self.z * quat.z
    
    def relative_angle(self, quat):
        # Angle between two quaternion
        rel = quat.inv() * self
        return 2 * numpy.arctan2(numpy.linalg.norm(rel.vector[1:4]), rel.vector[0]) / numpy.pi * 180.0
    
    def quat_from_axis_angle(self, angle):
        # Convert a rotation defined by an axis and angle into a quaternion
        # (angle in radians)
        cos = numpy.cos(angle / 2)
        sin = numpy.sin(angle / 2)
        return Quaternion(cos, self.x * sin, self.y * sin, self.z * sin)
    
    def quat_from_rot_matrix(self, R):
        # Validate the rotation matrix R is 3x3
        # Please ensure the rotation matrix R is orthogonal
        #  and has a determinant of +1 before converting
        if not R.shape == (3, 3):
            return
        
        r11, r12, r13 = R[0][0], R[0][1], R[0][2]
        r21, r22, r23 = R[1][0], R[1][1], R[1][2]
        r31, r32, r33 = R[2][0], R[2][1], R[2][2]
        
        tr = r11 + r22 + r33
        
        if tr > 0:
            s = numpy.sqrt(1.0 + tr) * 2
            w = s / 4
            x = (r32 - r23) / s
            y = (r13 - r31) / s
            z = (r21 - r12) / s
            
        elif r11 > r22 and r11 > r33:
            s = numpy.sqrt(1.0 + r11 - r22 - r33) * 2
            w = (r32 - r23) / s
            x = s / 4
            y = (r12 + r21) / s
            z = (r13 + r31) / s
            
        elif r22 > r33:
            s = numpy.sqrt(1.0 + r22 - r11 - r33) * 2
            w = (r13 - r31) / s
            x = (r12 + r21) / s
            y = s / 4
            z = (r23 + r32) / s    
            
        else:
            s = numpy.sqrt(1.0 + r33 - r11 - r22) * 2
            w = (r21 - r12) / s
            x = (r13 + r31) / s
            y = (r23 + r32) / s
            z = s / 4
            
        return Quaternion(w, x, y, z)
            
    def quat_to_euler(self):
        # Convert quaternion to euler angles (roll, pitch, yaw)
        # [w, x, y, z] -> [roll, pitch, yaw]
        w, x, y, z = self.vector
        
        # Roll (x-axis rotation)
        sinr_cosp = 2.0 * (w * x + y * z)
        cosr_cosp = 1.0 - 2.0 * (x**2 + y**2)
        roll = numpy.arctan2(sinr_cosp, cosr_cosp) / numpy.pi * 180.0
        roll = (roll + 180) % 360 - 180
        # if sinr_cosp < 0:
            # roll += 180.0
        
        # print(f'{w:1.2f}\t{x:1.2f}\t{y:1.2f}\t{z:1.2f}\t{sinr_cosp:1.2f}\t{cosr_cosp:1.2f}\t{roll:1.2f}')
        
        # Pitch (y-axis rotation)
        sinp = 2.0 * (w * y - z * x)
        if abs(sinp) >= 1:
            pitch = numpy.sign(sinp) * (numpy.pi / 2) / numpy.pi * 180.0  # use 90 degrees if out of range
        else:
            pitch = numpy.arcsin(sinp) / numpy.pi * 180.0

        pitch = (pitch + 180) % 360 - 180

        # Yaw (z-axis rotation)
        siny_cosp = 2 * (w * z + x * y)
        cosy_cosp = 1 - 2 * (y**2 + z**2)
        yaw = numpy.arctan2(siny_cosp, cosy_cosp) / numpy.pi * 180.0
        yaw = (yaw + 180) % 360 - 180
        
        # return numpy.degrees(roll), numpy.degrees(pitch), numpy.degrees(yaw)
        return roll, pitch, yaw
    
    def quat_to_euler_backup(self):
        # Convert quaternion to euler angles (roll, pitch, yaw)
        # [w, x, y, z] -> [roll, pitch, yaw]
        w, x, y, z = self.vector
        
        # Roll (x-axis rotation)
        sinr_cosp = 2 * (w * x + y * z)
        cosr_cosp = 1 - 2 * (x**2 + y**2)
        roll = numpy.arctan2(sinr_cosp, cosr_cosp) / numpy.pi * 180.0
        
        print(f'{sinr_cosp:1.2f}\t{cosr_cosp:1.2f}\t{roll:1.2f}')
        
        # Pitch (y-axis rotation)
        sinp = 2 * (w * y - z * x)
        if abs(sinp) >= 1:
            pitch = numpy.sign(sinp) * (numpy.pi / 2) / numpy.pi * 180.0  # use 90 degrees if out of range
        else:
            pitch = numpy.arcsin(sinp) / numpy.pi * 180.0

        # Yaw (z-axis rotation)
        siny_cosp = 2 * (w * z + x * y)
        cosy_cosp = 1 - 2 * (y**2 + z**2)
        yaw = numpy.arctan2(siny_cosp, cosy_cosp) / numpy.pi * 180.0
        
        # return numpy.degrees(roll), numpy.degrees(pitch), numpy.degrees(yaw)
        return roll, pitch, yaw
    
    def euler_to_quat(self, roll, pitch, yaw):
        # Convert angles to radians
        # [roll, pitch, yaw] -> [w, x, y, z]
        roll = roll * numpy.pi / 180.0
        pitch = pitch * numpy.pi / 180.0
        yaw = yaw * numpy.pi / 180.0
        
        cy = numpy.cos(yaw * 0.5)
        sy = numpy.sin(yaw * 0.5)
        cp = numpy.cos(pitch * 0.5)
        sp = numpy.sin(pitch * 0.5)
        cr = numpy.cos(roll * 0.5)
        sr = numpy.sin(roll * 0.5)
        
        w = cr * cp * cy + sr * sp * sy
        x = sr * cp * cy - cr * sp * sy
        y = cr * sp * cy + sr * cp * sy
        z = cr * cp * sy - sr * sp * cy
        
        self.set_vector(w, x, y, z)
        
    def rotate(self, vector):
        # Rotate a 3D vector using the quaternion
        # Return with the updated 3D vector
        q_v = Quaternion(0, vector[0], vector[1], vector[2])
        q_conj = self.inv()
        q_rot = self * q_v * q_conj
        return q_rot.vector[1:]
    

class Butterworth:
    FILTER_BUFFER_SIZE = 3
    
    def __init__(self):
        # Setup buffer
        self.buffer_in = collections.deque(numpy.zeros(self.FILTER_BUFFER_SIZE), maxlen=self.FILTER_BUFFER_SIZE)
        self.buffer_out = collections.deque(numpy.zeros(self.FILTER_BUFFER_SIZE), maxlen=self.FILTER_BUFFER_SIZE)
        # Setup coefficients
        self.a = numpy.zeros(self.FILTER_BUFFER_SIZE)
        self.b = numpy.zeros(self.FILTER_BUFFER_SIZE)
        
        self.set_enable(True)
        
    def set_enable(self, val):
        if type(val) is bool:
            self.enable = val

    def butter(self, sample_freq, cutoff=None, mode='low'):
            
        # polynomial coefficient of 2nd order Butterworth filter
        ita = 1.0 / numpy.tan(numpy.pi * cutoff / sample_freq)
        q = numpy.sqrt(2.0)
        self.b[0] = 1.0 / (1.0 + q * ita + ita * ita)
        self.b[1] = self.b[0] * 2
        self.b[2] = self.b[0]
        self.a[1] = 2.0 * (ita * ita - 1.0) * self.b[0]
        self.a[2] = -(1.0 - q * ita + ita * ita) * self.b[0]
        
        if mode == 'high':
            self.b[0] = self.b[0] * ita * ita
            self.b[1] = self.b[1] * ita * ita * -1
            self.b[2] = self.b[2] * ita * ita
        
        return self.a, self.b
    
    def feed(self, data_in):
        if self.enable:
            
            # Register and shift data into buffer array
            self.buffer_in.appendleft(data_in)
            self.buffer_out.appendleft(0.0)
            
            # Compute filtered data using 2nd order Butterworth Filter
            for i in range(3):
                self.buffer_out[0] += self.b[i] * self.buffer_in[i]
                self.buffer_out[0] += self.a[i] * self.buffer_out[i]
        
            # Filtered
            return self.buffer_out[0]
        
        else:
            return None
        
    def filt(self, data):
        if self.enable:
            
            # Prepare filter output data array
            output = numpy.zeros(data.shape)
            
            # Feed input data into filter and put output to data array
            for i in range(len(data)):
                output[i] = self.feed(data[i])
                
            return output
        
        else:
            return None
    