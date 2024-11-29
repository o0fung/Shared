import numpy
import run
import collections

from scipy.signal import welch


BUFFER_SIZE_ICM = 200
BUFFER_SIZE_EMG = 2000
BUFFER_SIZE_RMS = 100
        
class EMGS:
    list_str_icm_mode = [
        'raw_acc',
        'cal_acc',
        'lin_acc',
        'raw_gyr',
        'cal_gyr',
        'raw_mag',
        'cal_mag',
        'quat_vec',
        'quat_mag',
    ]
    list_str_imu_sensor = {
        1: 'acc',
        4: 'gyr',
        6: 'mag',
    }
    list_str_battery = {
        'low': 3.1 * 50,
        'high': 4.15 * 50,
    }

    list_str_channel = {}
    list_str_channel['icm'] = [
        'icmT',
        'accX',
        'accY',
        'accZ',
        'accT',
        'gyrX',
        'gyrY',
        'gyrZ',
        'gyrT',
        'magX',
        'magY',
        'magZ',
        'magT',
    ]
    list_str_channel['emg'] = [
        'emgT',
        'emg',
        'rms',
        'mnf',
        'mdf',
    ]
    
    def __init__(self, addr):
        self.client = None
        self.addr = addr
        
        self.is_connected = False
        self.is_streaming = False
        self.is_charging = False
        
        self.name = ''
        self.ver_fw = ''
        self.ver_hw = ''
        self.ver_sw = ''
        self.timestamp = 0
        self.battery = 0
        
        self.emg_mode = 0
        self.icm_mode = {}
        for mode in self.list_str_icm_mode:
            self.icm_mode[mode] = False
        
        self.t0 = 0.0
        self.data = {}
        self.data_types = {}
        self.data_count = {}
        
        self.buffer = {}
        
    def set_zero_data(self):
        
        # Prepare data type of ICM sensor
        self.data_types['icm'] = []
        for ch in self.list_str_channel['icm']:
            self.data_types['icm'].append((ch, 'f4'))
            
        # Prepare data type of EMG sensor
        self.data_types['emg'] = []
        for ch in self.list_str_channel['emg']:
            self.data_types['emg'].append((ch, 'f4'))
        
        # Set zero data count for each sensor
        self.data_count['icm'] = 0
        self.data_count['emg'] = 0
        
        self.t0 = 0.0
        
        # Initiate empty structured array for each sensor
        self.data['icm'] = numpy.empty(0, dtype=self.data_types['icm'])
        self.data['emg'] = numpy.empty(0, dtype=self.data_types['emg'])
        
        for dev in self.list_str_channel['icm']:
            self.buffer[dev] = collections.deque([0 for _ in range(BUFFER_SIZE_ICM)], maxlen=BUFFER_SIZE_ICM)
        
        for dev in self.list_str_channel['emg']:
            self.buffer[dev] = collections.deque([0 for _ in range(BUFFER_SIZE_EMG)], maxlen=BUFFER_SIZE_EMG)
        
        self.buffer['rms100'] = collections.deque([0 for _ in range(BUFFER_SIZE_RMS)], maxlen=BUFFER_SIZE_RMS)
        
    def add_zero_data(self, dev, length, missing_value=None):
        # An option to fill in the new array with predefined missing value
        if missing_value is None:
            missing_value = 0
            
        # Prepare an empty array filled with missing value
        new_extension = numpy.empty(length, dtype=self.data_types[dev])
        for ch in self.list_str_channel[dev]:
            new_extension[ch] = missing_value
        
        # Concatenate the prepared array to the main array
        # And update the data count
        self.data[dev] = numpy.concatenate((self.data[dev], new_extension))
        self.data_count[dev] += length
        
    def add_data_buffer(self, dev, val):
        self.buffer[dev].append(val)
        
    def compute_emg_features(self, dt):
        rms = numpy.sqrt(numpy.mean(numpy.array(self.buffer['rms100']) ** 2))
        self.data['emg']['rms'][dt] = rms
        self.add_data_buffer('rms', rms)

        # Compute the power spectrum density from FFT of EMG
        frequencies, power_spectrum = welch(self.buffer['emg'], fs=1000)
        
        if not power_spectrum.all():
            return [0], [0]
                
        # Compute the MNF of the EMG
        mean_frequency = numpy.sum(power_spectrum * frequencies) / numpy.sum(power_spectrum)
        self.data['emg']['mnf'][dt] = mean_frequency
        
        # Calculate the cumulative sum of the power spectrum
        cumulative_power = numpy.cumsum(power_spectrum)
        # Find the median frequency
        total_power = cumulative_power[-1]
        median_freq_index = numpy.where(cumulative_power >= total_power / 2)[0][0]
        median_frequency = frequencies[median_freq_index]
        self.data['emg']['mdf'][dt] = median_frequency
            
        return frequencies, power_spectrum
        
    def data_processing(self):
        # Trigger the data processing algorithm
        data = run.Data(self.data)
        data.work_on_ui()
        