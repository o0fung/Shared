import traceback
import os
import time
import argparse
import numpy

from matplotlib import pyplot


class Data:
    def __init__(self, data, skip_filter=False):
        self.path = ''
        
        if skip_filter:
            # Load data that has already been processed
            self.icm = data['icm']
            self.emg = data['emg']

        else:
            # Load the data to numpy structured array
            # Filter the missing data, i.e. rows where Time and Data are both zeros
            self.icm_0 = data['icm'][data['icm']['icmT'] != 0]
            self.emg_0 = data['emg'][data['emg']['emgT'] != 0]
            
            # Unit of ICM time is 10ms, so should multipy it by 10
            self.icm_0['icmT'] *= 10.0

            # Previously the first row with Time=0 have also been filtered
            # Now recover the first row for complete data set
            self.icm = numpy.concatenate(([data['icm'][0]], self.icm_0))
            self.emg = numpy.concatenate(([data['emg'][0]], self.emg_0))
            
    def set_path(self, path):
        self.path = path        
    
    def work_on_ui(self):
        # Suppose to run in User Interface
        
        try:
            self.save_data()
            self.display()
            
        except Exception as e:
            traceback.print_exc()
            
        except SyntaxError:
            pass
        
    def work_on_cli(self):
        # Suppose to run in Command Prompt Terminal
        
        try:
            self.display()
            self.save_fig(self.path)
            
            pyplot.show()
            
        except Exception as e:
            traceback.print_exc()
            
        except SyntaxError:
            pass
        
    def work_in_progress(self):
        # Running test and model
        
        try:
            pass
            
        except Exception as e:
            traceback.print_exc()
        
    def save_data(self, path=None):
        # Save all data to files for future use
        
        if path is not None:
            path_to_data = path
        else:
            # Setup a new folder with the current timestamp
            path_to_data = os.path.join(os.path.dirname(__file__), time.strftime('data_%Y%m%d%H%M%S'))
            if not os.path.exists(path_to_data):
                os.mkdir(path_to_data)
        
        # Save ICM data as CSV file
        with open(os.path.join(path_to_data, 'icm.csv'), 'wb') as f:
            numpy.savetxt(f, self.icm, delimiter=',', header=','.join(self.icm.dtype.names), fmt='%f'+',%f'*(len(self.icm.dtype.names)-1), comments='')
        
        # Save EMG data as CSV file
        with open(os.path.join(path_to_data, 'emg.csv'), 'wb') as f:
            numpy.savetxt(f, self.emg, delimiter=',', header=','.join(self.emg.dtype.names), fmt='%f'+',%f'*(len(self.emg.dtype.names)-1), comments='')
            
        # Save ICM data as Numpy data file
        with open(os.path.join(path_to_data, 'icm.data'), 'wb') as f:
            numpy.save(f, self.icm, allow_pickle=True)
            
        # Save EMG data as Numpy data file
        with open(os.path.join(path_to_data, 'emg.data'), 'wb') as f:
            numpy.save(f, self.emg, allow_pickle=True)
            
    def save_fig(self, path=None):
        # Save all data to figure for future use
        
        if path is not None:
            path_to_data = path
        
        else:
            # Setup a new folder with the current timestamp
            path_to_data = os.path.join(os.path.dirname(__file__), time.strftime('data_%Y%m%d%H%M%S'))
            if not os.path.exists(path_to_data):
                os.mkdir(path_to_data)
            
        # Save figure as png file
        with open(os.path.join(path_to_data, 'fig.png'), 'wb') as f:
            pyplot.savefig(f)
            
    def display(self):
        # Display all data in one figure for reference
        
        with pyplot.ion():
        
            fig, ax = pyplot.subplots(4, 1, sharex=True, num='Display All')
            
            ax[0].plot(self.icm['icmT'], self.icm['accX'], color='b')
            ax[0].plot(self.icm['icmT'], self.icm['accY'], color='r')
            ax[0].plot(self.icm['icmT'], self.icm['accZ'], color='g')
            ax[0].set_ylabel('Acc (g)')
            ax[0].set_title('Accelerometer (ACC)')
            ax[0].grid(True, which='both', axis='both')
            
            ax[1].plot(self.icm['icmT'], self.icm['gyrX'], color='b')
            ax[1].plot(self.icm['icmT'], self.icm['gyrY'], color='r')
            ax[1].plot(self.icm['icmT'], self.icm['gyrZ'], color='g')
            ax[1].set_ylabel('Gyr (deg/s)')
            ax[1].set_title('Gyroscope (GYR)')
            ax[1].grid(True, which='both', axis='both')
            
            ax[2].plot(self.icm['icmT'], self.icm['magX'], color='b')
            ax[2].plot(self.icm['icmT'], self.icm['magY'], color='r')
            ax[2].plot(self.icm['icmT'], self.icm['magZ'], color='g')
            ax[2].set_ylabel('Mag (mT)')
            ax[2].set_title('Magnetometer (MAG)')
            ax[2].grid(True, which='both', axis='both')
            
            ax[3].plot(self.emg['emgT'], self.emg['emg'])
            ax[3].set_ylabel('EMG (mV)')
            ax[3].set_xlabel('Time (ms)')
            ax[3].set_title('Electromyography (EMG)')
            ax[3].grid(True, which='both', axis='both')
            
            
if __name__ == '__main__':
    # parse from command line the target data directory path
    parser = argparse.ArgumentParser(description='Run and test the algorithm')
    parser.add_argument('path', help='target directory path to data')
    args = vars(parser.parse_args())
    
    print(f'>> Run and test algorithm')
    print(f'>> Data path: {args["path"]}')
    print(f'>> Data loading successful.')
    
    buffer = {'icm': None, 'emg': None}

    # Load ICM data
    with open(os.path.join(args['path'], 'icm.data'), 'rb') as f:
        buffer['icm'] = numpy.load(f, allow_pickle=True)
        
    # Load EMG data
    with open(os.path.join(args['path'], 'emg.data'), 'rb') as f:
        buffer['emg'] = numpy.load(f, allow_pickle=True)
    
    # Run the data algorithm
    data = Data(buffer, skip_filter=True)
    data.set_path(args['path'])
    
    # data.work_on_cli()
    
    data.work_in_progress()
    