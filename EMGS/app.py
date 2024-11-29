import os
import sys
import time
import struct
import asyncio
import qasync
import bleak
import collections
import traceback
import importlib
import run
import pyqtgraph

import emgs

from PyQt6 import QtWidgets, uic, QtCore

# Saved EMGS UUID
# List of devices that will be displayed for selection after reset
default_emgs_address = {
    'EMGS-9402': '940275A4-0385-85EB-8FEC-C69E78086B09',
    'EMGS-4258': '4258E3C2-9FBC-91E2-34B6-521A5F9896BD',
    
}
# Available BLE services
emgs_uuid = {
    'WRITE': '6e400002-b5a3-f393-e0a9-e50e24dcca9e',    # Nordic UART RX
    'NOTIFY': '6e400003-b5a3-f393-e0a9-e50e24dcca9e',    # Nordic UART TX
}
 

class UserInterface(QtWidgets.QMainWindow):
    """ PyQt6 UI application
        - Scan nearby BLE devices
        - Connect to selected BLE device
        - Read and write to the BLE services
    """
    
    def __init__(self, path):
        super().__init__()
        
        # Set up UI and variables
         
        uic.loadUi(path, self)
    
        self.setup_vars()
        
        self.setup_graphs()
        
        # Set up connects and slots
        
        self.btn_scan.clicked.connect(self.handle_scan)
        self.btn_connect.clicked.connect(self.handle_disconnect)
        self.btn_clear.clicked.connect(self.handle_clear_scanned_list)
        self.btn_send.clicked.connect(self.handle_manual_input)
        self.btn_time_sync.clicked.connect(self.handle_time_sync)
        self.btn_update.clicked.connect(self.handle_manual_update)
        self.btn_stream_start.clicked.connect(lambda: self.handle_send('A5'))
        self.btn_stream_stop.clicked.connect(lambda: self.handle_send('A7'))
        self.btn_data_processing.clicked.connect(self.handle_data_processing)
        self.btn_update_library.clicked.connect(self.handle_update_library)
        
        self.chkbox_raw_acc.clicked.connect(lambda: self.clicked_checkbox_icm(0))
        self.chkbox_cal_acc.clicked.connect(lambda: self.clicked_checkbox_icm(1))
        self.chkbox_lin_acc.clicked.connect(lambda: self.clicked_checkbox_icm(2))
        self.chkbox_raw_gyr.clicked.connect(lambda: self.clicked_checkbox_icm(3))
        self.chkbox_cal_gyr.clicked.connect(lambda: self.clicked_checkbox_icm(4))
        self.chkbox_raw_mag.clicked.connect(lambda: self.clicked_checkbox_icm(5))
        self.chkbox_cal_mag.clicked.connect(lambda: self.clicked_checkbox_icm(6))
        self.chkbox_quat_vec.clicked.connect(lambda: self.clicked_checkbox_icm(7))
        self.chkbox_quat_mag.clicked.connect(lambda: self.clicked_checkbox_icm(8))
        self.chkbox_emg.clicked.connect(lambda x: self.clicked_checkbox_emg(x))
        
        self.input_name.editingFinished.connect(self.change_ble_name)
        
        self.list_devices.itemSelectionChanged.connect(self.change_state_selecting_ble)
        
        # Set up widgets states
        
        self.btn_clear.clicked.emit()
        
    def setup_vars(self):
        self.vars = {}
        
        self.vars['devices_list'] = []
        self.vars['t0'] = time.time()
        self.vars['current_dev_addr'] = None
        self.vars['list_emgs'] = list(default_emgs_address.items())
        
        self.sensors = {}
        
    def setup_graphs(self):
        # Prepare drawing pen with colors
        self.plot_pen = {}
        self.plot_pen['x'] = pyqtgraph.mkPen(color=(0, 0, 255))         # blue
        self.plot_pen['y'] = pyqtgraph.mkPen(color=(255, 0, 0))         # red
        self.plot_pen['z'] = pyqtgraph.mkPen(color=(0, 255, 0))         # green
        self.plot_pen['e'] = pyqtgraph.mkPen(color=(255, 255, 255))     # white
        self.plot_pen['r'] = pyqtgraph.mkPen(color=(255, 0, 0))         # red
        # Prepare text font with color and size
        self.font_style = {'color': 'white', 'font-size': '10px'}
        
        self.graphs = {}        # Store graph widget
        self.lines = {}         # Store line objects
        self.legends = {}
        
        for dev in ['acc', 'gyr', 'mag', 'emg', 'freq']:
            self.graphs[dev] = pyqtgraph.PlotWidget()
            self.graphs[dev].setBackground('k')
            self.graphs[dev].showGrid(x=True, y=True)
            
            if dev in ['acc', 'gyr', 'mag']:
                
                self.lines[dev] = {}
                
                for axe in ['x', 'y', 'z']:
                    self.lines[dev][axe] = self.graphs[dev].plot(
                        [0, 1],
                        [0, 1],
                        name=f'{dev.upper()}{axe.upper()}',
                        pen=self.plot_pen[axe],
                    )
                
                self.graphs[dev].setLabel('bottom', 'Time (ms)', **self.font_style)
                self.layout_graph_icm.addWidget(self.graphs[dev])
                
            else:
                self.lines[dev] = self.graphs[dev].plot(
                    [0, 1],
                    [0, 1],
                    name=f'{dev.upper()}',
                    pen=self.plot_pen['e'],
                )
                
                if dev in ['emg']:
                    self.lines['rms'] = self.graphs[dev].plot(
                        [0, 1],
                        [0, 1],
                        name='RMS',
                        pen=self.plot_pen['r'],
                    )
                    self.graphs[dev].setLabel('bottom', 'Time (ms)', **self.font_style)
                    
                else:
                    self.graphs[dev].setLabel('bottom', 'Frequency (Hz)', **self.font_style)

                self.layout_graph_emg.addWidget(self.graphs[dev])
        
        self.graphs['acc'].setTitle('Accelerometer (ACC)', **self.font_style)
        self.graphs['acc'].setLabel('left', 'Acc (g)', **self.font_style)
        self.graphs['acc'].setYRange(-4, 4)
        
        self.graphs['gyr'].setTitle('Gyroscope (GYR)', **self.font_style)
        self.graphs['gyr'].setLabel('left', 'Gyr (deg/s)', **self.font_style)
        self.graphs['gyr'].setYRange(-2000, 2000)
        
        self.graphs['mag'].setTitle('Magnetometer (MAG)', **self.font_style)
        self.graphs['mag'].setLabel('left', 'Mag (mT)', **self.font_style)
        self.graphs['mag'].setYRange(-1000, 1000)
        
        self.graphs['emg'].setTitle('Electromygraphy (EMG)', **self.font_style)
        self.graphs['emg'].setLabel('left', 'EMG (mV)', **self.font_style)
        self.graphs['emg'].setYRange(-1.5, 1.5)
        
        self.graphs['freq'].setTitle('Frequency Spectrum', **self.font_style)
        self.graphs['freq'].setLabel('left', '', **self.font_style)
        self.graphs['freq'].setYRange(0, 0.002)
        
        self.legends['freq'] = pyqtgraph.TextItem('MNF = ?, MDF = ?')
        self.legends['freq'].setPos(0, 0.002)
        self.graphs['freq'].addItem(self.legends['freq'])
        
    def show_message(self, txt):
        # Print message to stdout and text widget
        output = time.strftime(f'%Y-%m-%d %H:%M:%S >> {txt}')
        print(output)
        self.txt_output.append(output)
        
    def show_warning(self, txt):
        # Print warning to stdout and text widget
        msg_err = f'An error occurred: {str(txt)}'
        self.show_message(msg_err)
        QtWidgets.QMessageBox.critical(self, 'Error', msg_err)
        
    def handle_clear_scanned_list(self):
        # Clear the list of scanned BLE devices
        self.list_devices.blockSignals(True)
        self.list_devices.clear()
        self.list_devices.blockSignals(False)
        
        # Replace the list with the default list of connected BLE devices
        self.vars['devices_list'] = []
        for emgs_name, emgs_addr in self.vars['list_emgs']:
            self.list_devices.addItem(f'{emgs_name}\t - {emgs_addr}')
            self.vars['devices_list'].append((emgs_name, emgs_addr))
            
    def handle_clear_system_status(self):
        self.input_name.setText('-')
        self.str_battery.setText('-')
        self.str_charge.setText('-')
        self.str_timestamp.setText('-')
        self.str_ver_fw.setText('-')
        self.str_ver_hw.setText('-')
        self.str_ver_sw.setText('-')
        self.chkbox_raw_acc.setChecked(False)
        self.chkbox_cal_acc.setChecked(False)
        self.chkbox_lin_acc.setChecked(False)
        self.chkbox_raw_gyr.setChecked(False)
        self.chkbox_cal_gyr.setChecked(False)
        self.chkbox_raw_mag.setChecked(False)
        self.chkbox_cal_mag.setChecked(False)
        self.chkbox_quat_vec.setChecked(False)
        self.chkbox_quat_vec.setChecked(False)
        self.chkbox_emg.setCheckState(QtCore.Qt.CheckState.Unchecked)
        
    @qasync.asyncSlot()
    async def handle_scan(self):
        # To scan and get a list of BLE devices
        
        self.list_devices.setCurrentRow(-1)
        self.list_devices.clear()
        list_of_scanned_devices = collections.defaultdict(list)

        self.show_message('Scanning nearby BLE devices...')
        
        try:
            # Scan nearby BLE devices (takes some time ~10 sec)
            # Group scanned devices depends on the availability of name
            devices = await bleak.BleakScanner.discover()
            for device in devices:
                types_of_device = 'named' if device.name is not None else 'unnamed'
                list_of_scanned_devices[types_of_device].append(device)
            
            # Sort the list of scanned devices according to name or address depending on its group
            # Organize the sorted list of devices to a dedicated list
            self.vars['devices_list'] = []
            for t_dev in ['named', 'unnamed']:
                list_of_scanned_devices[t_dev].sort(key=lambda x, t=t_dev: x.name if t == 'named' else x.address)
                list_of_sorted_device = [(dev.name, dev.address) for dev in list_of_scanned_devices[t_dev]]
                self.vars['devices_list'].extend(list_of_sorted_device)
                
            # Display the organized list of devices onto the listbox for selection
            for device in self.vars['devices_list']:
                self.list_devices.addItem(f'{device[0]}\t - {device[1]}')
                
        except Exception as e:
            # Capture any exceptions and errors
            str_err = traceback.format_exc()
            self.show_warning(str_err)
            
        self.show_message('Scan completed.')
        
    @qasync.asyncSlot()
    async def handle_connect(self):
        # To connect selected BLE device
        
        # Get the selected device from the selected row
        # Check if the address is registered
        addr = self.vars['current_dev_addr']
        if addr not in self.sensors:
            self.sensors[addr] = emgs.EMGS(addr)
            
        if self.sensors[addr].is_connected:
            return
        
        self.sensors[addr].is_connected = True
        
        self.show_message(f'Selected device:\n {addr}')
        self.show_message('Connecting to selected BLE devices...')
        
        try:
            # Start the BLE connection using Bleak client
            async with bleak.BleakClient(addr) as self.sensors[addr].client:
                
                # Connect to BLE device
                await self.sensors[addr].client.connect()
                if not self.sensors[addr].client.is_connected:
                    return
                
                # Start notification of the BLE after connection successful
                await self.sensors[addr].client.start_notify(emgs_uuid['NOTIFY'], lambda client, data, address=addr: self.handle_notify(client, data, address))
                
                # Set up connection flag to control connect state
                # Display time elapse since connected
                self.vars['t0'] = time.time()
                self.change_state_connecting_ble(True)
                self.show_message(time.strftime(f'Connected. ({addr[:4]})'))
                
                # Update the list of EMGS connected
                if addr not in [dev[1] for dev in self.vars['list_emgs']]:
                    # Get more info about the selected BLE device to be connected
                    selected_index = self.list_devices.row(self.list_devices.selectedItems()[0])
                    target_dev_name, addr = self.vars['devices_list'][selected_index]
                    self.vars['list_emgs'].append((target_dev_name, addr))
                
                # Clear and update the list of BLE devices
                self.handle_clear_scanned_list()
                # Highlight the selected device to start update system status
                dev_index = [dev[1] for dev in self.vars['list_emgs']].index(addr)
                self.list_devices.setCurrentRow(dev_index)
                
                await self.handle_sleep(dt=3)
                
                # Assign tasks to do that run in a loop
                # Use connection flag to control loop
                while self.sensors[addr].is_connected:
                    await self.handle_sleep(dt=1)
                    
                try:
                    # User decided to end the loop and disconnect from BLE device
                    # Stop notification of the BLE before disconnection
                    await self.sensors[addr].client.stop_notify(emgs_uuid['NOTIFY'])
                    await self.sensors[addr].client.disconnect()
                    # Allow some time for the BLE to end completely
                    await self.handle_sleep(dt=3)
                    
                except AttributeError as e:
                    # When there was errors in the while loop
                    # with immature termination of communication
                    pass
                
                self.change_state_connecting_ble(False)
                self.handle_clear_system_status()
                self.show_message(f'Disconnected. ({addr[:4]})')
        
        except bleak.exc.BleakDeviceNotFoundError as e:
            # When the BLE device is not turned on
            # Failed to search the device
            self.sensors[addr].is_connected = False
            self.show_warning(e)
        
        except Exception as e:
            # Capture any exceptions and errors
            self.sensors[addr].is_connected = False
            str_err = traceback.format_exc()
            self.show_warning(str_err)

    def handle_disconnect(self):
        # Change the connection state to OFF
        # Exit the BLE loop and then continue to disconnect device
        addr = self.vars['current_dev_addr']
        self.sensors[addr].is_connected = False
        self.show_message(f'Disconnecting... ({addr[:4]})')
        
    @qasync.asyncSlot()
    async def handle_notify(self, client, data, addr):
        # Get headers and length of data packet
        # Validate format of the data packet
        n = len(data)
        if n >= 3 and chr(data[0]) == 'S':
            
            # Data packet is for general commands
            if chr(data[1]) == 'A':
                cmd = chr(data[2])   # Command byte
                
                if cmd in ['0', '3', '5', '7', 'a']:    # Get battery and charge
                    batt_lo = self.sensors[addr].list_str_battery['low']
                    batt_hi = self.sensors[addr].list_str_battery['high']
                    batt_val = (data[3] - batt_lo) / (batt_hi - batt_lo) * 100.0
                    batt_val = batt_val if batt_val <= 100.0 else 100.0
                    batt_val = batt_val if batt_val >= 0.0 else 0.0
                    self.sensors[addr].battery = batt_val
                    self.str_battery.setText(f'{batt_val:1.0f}%')
                    
                    charging = (data[4] == 1)
                    self.sensors[addr].is_charging = charging
                    self.str_charge.setText('Yes' if charging else 'No')
                    
                if cmd == '0':      # Get version
                    self.sensors[addr].ver_fw = f'{data[5]}.{data[6]}'
                    self.str_ver_fw.setText(self.sensors[addr].ver_fw)
                    
                    self.sensors[addr].ver_hw = f'{data[7]}.{data[8]}'
                    self.str_ver_hw.setText(self.sensors[addr].ver_hw)
                    
                elif cmd == '3':      # Get time sync
                    ts = struct.unpack('Q', data[5:13])[0]
                    self.sensors[addr].timestamp = ts
                    self.str_timestamp.setText(time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(ts / 1000.0)))
                    
                elif cmd == 'a':      # Get version DSP
                    self.sensors[addr].ver_sw = f'{data[5]}.{data[6]}.{data[7]}'
                    self.str_ver_sw.setText(self.sensors[addr].ver_sw)
                    
                elif cmd == 'G':      # Get timestamp
                    ts = struct.unpack('Q', data[3:11])[0]
                    self.sensors[addr].timestamp = ts
                    self.str_timestamp.setText(time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(ts / 1000.0)))
                    
                elif cmd == 'X':      # Get ICM Enable Mode
                    mode_code = data[3]
                    mode_str = self.sensors[addr].list_str_icm_mode[mode_code]
                    if 0 <= mode_code <= 8:
                        val = (data[4] == 1)
                        
                        self.sensors[addr].icm_mode[mode_str] = val
                        self.change_state_checkbox_icm(mode_code, val)
                    
                elif cmd == 'x':      # Get EMG Enable Mode
                    mode_code = data[3]
                    if 0 <= mode_code <= 2:
                        self.sensors[addr].emg_mode = mode_code
                        self.change_state_checkbox_emg(mode_code)
                    
                elif cmd == 'K':      # Get BLE advertising name
                    txt = data[3:15]
                    name = ''
                    
                    for ch in txt:
                        if ch == 0:
                            break
                        # Accept the string as name until reaching \0
                        name += chr(ch)
                    
                    self.sensors[addr].name = name
                    self.input_name.setText(name)
                
                elif cmd == '5':
                    self.show_message(f'Streaming Start... ({addr[:4]})')
                    
                    self.sensors[addr].is_streaming = True
                    self.sensors[addr].set_zero_data()
                    
                elif cmd == '7':
                    if self.sensors[addr].is_streaming:
                        self.sensors[addr].is_streaming = False
                        
                        self.show_message(f'Streaming Stop... ({addr[:4]})')
                        
                        self.handle_data_processing()
                
                elif cmd in ['W', 'w', 'F']:
                    print('OK                                                          \r', end='')
                
                else:
                    print(struct.unpack(f'{n}B', data))
                    
            # Data packet is for charger notification
            elif chr(data[1]) == 'C':
                print(f'{addr} : Charging?                                       \r', end='')
            
            # Data packet is for charger notification
            elif chr(data[1]) == 'X':
                print(f'{addr} : Error!                                       \r', end='')
                      
            # Data packet is for ICM data transfer
            elif chr(data[1]) == 'I':
                # Decode data packet
                datalen = data[2]                                   # data packet length
                packet_id = struct.unpack('H', data[3:5])[0]        # packet id 
                sensor_type = data[5]                               # sensor type: 1=acc, 4=gyr, 6=mag
                timestamp = struct.unpack('Q', data[6:14])[0]       # timestamp: long 8 bytes
                samplingfreq = data[14]                             # sampling frequency: 100 Hz
                
                data_packet_count = int((datalen - 15) / 4)                     # how many xyz data in the packet
                readings = struct.unpack(data_packet_count * 'f', data[15:])    # decode unpack the data packets into xyz
                imu_sensor = self.sensors[addr].list_str_imu_sensor[sensor_type]
                
                # When the buffer needs to be extended to accomondate new data
                # Add zero padding with the following length
                extension_buffer_count = 25
                
                # For the first data packet after streaming start
                # Get ABSOULATE timestamp for the streaming start time
                if self.sensors[addr].t0 == 0.0:
                    self.sensors[addr].t0 = timestamp
                    self.sensors[addr].add_zero_data('icm', extension_buffer_count)
                    self.sensors[addr].add_zero_data('emg', extension_buffer_count)
                    dt = 0
                    
                else:
                    # Get RELATIVE timestamp of the subsequent data packets
                    dt = int(round((timestamp - self.sensors[addr].t0) / 10.0))
                    
                    if dt + extension_buffer_count >= self.sensors[addr].data_count['icm']:
                        adding_length = (((dt + extension_buffer_count - self.sensors[addr].data_count['icm']) // extension_buffer_count) + 1) * extension_buffer_count
                        self.sensors[addr].add_zero_data('icm', adding_length)
                        
                for i, reading in enumerate(readings):
                    # Fill in the expected data value of the frame
                    # Readings should be in sequence XYZ, XYZ, XYZ...
                    self.sensors[addr].data['icm'][f'{imu_sensor}{"XYZ"[i%3]}'][dt] = reading
                    self.sensors[addr].add_data_buffer(f'{imu_sensor}{"XYZ"[i%3]}', reading)
                    
                    # Fill in the expected time value of the frame
                    if i % 3 == 0:
                        self.sensors[addr].data['icm']['icmT'][dt] = dt
                        self.sensors[addr].data['icm'][f'{imu_sensor}T'][dt] = dt
                        self.sensors[addr].add_data_buffer(f'{imu_sensor}T', dt * 10.0 / 1000.0)
                        dt += 1     # Unit 10 milliseconds
                
                if addr == self.vars['current_dev_addr']:
                    # Update the ACC, GYR, MAG graphs
                    for i in range(3):
                        self.lines[imu_sensor]['xyz'[i]].setData(self.sensors[addr].buffer[f'{imu_sensor}T'],
                                                                self.sensors[addr].buffer[f'{imu_sensor}{"XYZ"[i]}'])
                        
            # Data packet is for EMG data transfer
            elif chr(data[1]) == 'E':
                # Decode data packet
                datalen = data[2]                                   # data packet length (constant 216)
                batt = data[3]                                      # m_batt
                charge_state = data[4]                              # charge_state
                packet_id = struct.unpack('H', data[5:7])[0]        # packet id 
                timestamp = struct.unpack('Q', data[7:15])[0]       # timestamp: long 8 bytes
                packet_mode_pos = data[15]                          # sampling frequency: 100 Hz
                snr = data[16]                                      # SNR
                rms = struct.unpack('>H', data[17:19])[0]           # RMS
                
                data_packet_count = int((datalen - 16) / 2)         # how many emg data in the packet
                readings = struct.unpack('>' + data_packet_count * 'H', data[19:])    # decode unpack the data packets into emg
                
                # When the buffer needs to be extended to accomondate new data
                # Add zero padding with the following length
                extension_buffer_count = 250
                
                # For the first data packet after streaming start
                # Get ABSOULATE timestamp for the streaming start time
                if self.sensors[addr].t0 == 0.0:
                    self.sensors[addr].t0 = timestamp
                    self.sensors[addr].add_zero_data('icm', extension_buffer_count)
                    self.sensors[addr].add_zero_data('emg', extension_buffer_count)
                    dt = 0
                    
                else:
                    # Get RELATIVE timestamp of the subsequent data packets
                    dt = int(round((timestamp - self.sensors[addr].t0)))
                    
                    if dt + extension_buffer_count >= self.sensors[addr].data_count['emg']:
                        adding_length = (((dt + extension_buffer_count - self.sensors[addr].data_count['emg']) // extension_buffer_count) + 1) * extension_buffer_count
                        self.sensors[addr].add_zero_data('emg', adding_length)
                
                for i, reading in enumerate(readings):
                    # Fill in the expected data value of the frame
                    emg = (reading / 65535.0 * 3.0 - 1.5) / 1200.0 * 1000.0
                    self.sensors[addr].data['emg']['emg'][dt] = emg
                    self.sensors[addr].add_data_buffer('emg', emg)
                    self.sensors[addr].add_data_buffer('rms100', emg)
                    
                    # Fill in the expected time value of the frame
                    self.sensors[addr].data['emg']['emgT'][dt] = dt
                    self.sensors[addr].add_data_buffer('emgT', dt / 1000.0)
                    dt += 1     # Unit 1 millisecond
                
                    frequencies, power_spectrum = self.sensors[addr].compute_emg_features(dt)
                    mnf = self.sensors[addr].data['emg']['mnf'][dt]
                    mdf = self.sensors[addr].data['emg']['mdf'][dt]
                    
                    output_txt = f'MNF = {mnf:1.0f}Hz\nMDF = {mdf:1.0f}Hz'
                    self.legends['freq'].setText(output_txt)
                
                # Update the EMG graph
                if addr == self.vars['current_dev_addr']:
                    self.lines['emg'].setData(self.sensors[addr].buffer['emgT'],
                                            self.sensors[addr].buffer['emg'])
                    self.lines['rms'].setData(self.sensors[addr].buffer['emgT'],
                                            self.sensors[addr].buffer['rms'])
                    
                    # Update the Frequency Spectrum of the EMG graph
                    self.lines['freq'].setData(frequencies, power_spectrum)
                    
                # Additional info for real-time monitoring battery level
                # EMG data packet contains battery info
                
                batt_lo = self.sensors[addr].list_str_battery['low']
                batt_hi = self.sensors[addr].list_str_battery['high']
                batt_val = (batt - batt_lo) / (batt_hi - batt_lo) * 100.0
                batt_val = batt_val if batt_val <= 100.0 else 100.0
                batt_val = batt_val if batt_val >= 0.0 else 0.0
                self.sensors[addr].battery = batt_val
                self.str_battery.setText(f'{batt_val:1.0f}%')
                
                charging = (charge_state == 1)
                self.sensors[addr].is_charging = charging
                self.str_charge.setText('Yes' if charging else 'No')
                
            # Other unattended data packets
            else:
                print(struct.unpack(f'{n}B', data))
        
    @qasync.asyncSlot()
    async def handle_sleep(self, dt):
        # Produce a time delay of duration dt
        await asyncio.sleep(dt)
        # Display a time elapse since connected message that update every second
        print(time.strftime("Time elapse: %H:%M:%S                                                \r", time.gmtime(time.time() - self.vars['t0'])), end='')
        
    @qasync.asyncSlot()
    async def handle_manual_input(self):
        # Write command packet to BLE device with user input strings
        
        # User can type in command string to execuate write command to BLE
        output = self.txt_command.toPlainText()
        await self.handle_send(output)
        
    @qasync.asyncSlot()
    async def handle_time_sync(self):
        # Set timestamp and form a string for send to BLE
        timestamp_list = list(struct.unpack('BBBBBBBB', int(time.time() * 1000).to_bytes(8, byteorder='little')))
        output = 'A,9'       # Command to set timestamp
        for ts_byte in timestamp_list:
            output += f',{str(ts_byte)}'
            
        await self.handle_send(output)
        
    @qasync.asyncSlot()
    async def handle_auto_update(self):
        await self.handle_send('A3')            # Get Time Sync (get battery, charge, timestamp)
        
    @qasync.asyncSlot()
    async def handle_manual_update(self):
        # Get current EMGS status
        await self.handle_send('A0')            # Get version
        await self.handle_send('Aa')            # Get version DSP
        await self.handle_send('AG')            # Get timestamp
        await self.handle_send('AK')            # Get BLE advertising name
        await self.handle_send('A,X,0')         # Get ICM Enable 0: Raw Acc
        await self.handle_send('A,X,1')         # Get ICM Enable 1: Cal Acc
        await self.handle_send('A,X,2')         # Get ICM Enable 2: Lin Acc
        await self.handle_send('A,X,3')         # Get ICM Enable 3: Raw Gyr
        await self.handle_send('A,X,4')         # Get ICM Enable 4: Cal Gyr
        await self.handle_send('A,X,5')         # Get ICM Enable 5: Raw Mag
        await self.handle_send('A,X,6')         # Get ICM Enable 6: Cal Mag
        await self.handle_send('A,X,7')         # Get ICM Enable 7: Quat Vec
        await self.handle_send('A,X,8')         # Get ICM Enable 8: Quat Mag
        await self.handle_send('Ax')            # Get EMG RMS Enable
        
    @qasync.asyncSlot()
    async def handle_send(self, cmd):
        # Write command packet to BLE device
        addr = self.vars['current_dev_addr']
        if not addr in self.sensors or not self.sensors[addr].is_connected:
            return
        
        try:
            # Validate input command is a list array
            if type(cmd) is not str:
                return
            
            # Accept a list of string or a list of byte integers
            output = []
            if ',' in cmd:
                # Format 1: Comma-separated string
                for item in cmd.split(','):
                    try:
                        output.append(int(item))
                    except ValueError:
                        output.append(ord(item))
            else:
                # Format 2: ASCII String
                for item in cmd:
                    output.append(ord(item))
            
            # Pack up the byte array
            # Write the input command to the BLE
            n = len(output)
            
            await self.sensors[addr].client.write_gatt_char(emgs_uuid['WRITE'], struct.pack(f'{n}B', *output))
            await asyncio.sleep(0.1)
            
        except bleak.exc.BleakError as e:
            # Can no longer send heartbeat, i.e. command A3 to the device
            self.show_warning(f'Connection lost to BLE ({addr[:4]})')
            self.handle_disconnect()
        
        except Exception as e:
            # Capture any exceptions and errors
            str_err = traceback.format_exc()
            self.show_message(f'Error in send packet to BLE ({addr[:4]}): {cmd}\n{str_err}')
            
    def handle_update_library(self):
        # Intended to revised the algorithm during runtime
        # So user can test the revised algorithm instantly
        importlib.reload(run)
        
        self.show_message('Updated algorithm library')
            
    def handle_data_processing(self):
        
        addr = self.vars['current_dev_addr']
        
        self.show_message(f'Start data processing... ({addr[:4]})')
        
        # Run the data processing algorithm
        self.sensors[self.vars['current_dev_addr']].data_processing()
        
        self.show_message(f'Finish data processing... ({addr[:4]})')
        
    def update_connected_emgs(self):
        # Get selected BLE device and address
        selected_item = self.list_devices.selectedItems()
        if not selected_item:
            return
        
        selected_index = self.list_devices.row(selected_item[0])
        target_dev_name, addr = self.vars['devices_list'][selected_index]
        
        # Add to the list of connected device if not already exist
        list_of_addr = [dev[1] for dev in self.vars['list_emgs']]
        if not addr in list_of_addr:
            self.vars['list_emgs'].append((target_dev_name, addr))
            list_of_addr = [dev[1] for dev in self.vars['list_emgs']]
        
        # Clear the list of BLE device
        self.handle_clear_scanned_list()
        
        # Highlight the selected device to start update system status
        dev_index = list_of_addr.index(addr)
        self.list_devices.setCurrentRow(dev_index)
            
    @qasync.asyncSlot()
    async def change_ble_name(self):
        # User has changed the input entry field
        name_str = self.input_name.text()[:12]

        # Prepare data packet to change BLE name
        output = 'A,F'
        for i in range(len(name_str)):
            output += f',{ord(name_str[i])}'
        
        await self.handle_send(output)
            
    def change_state_connecting_ble(self, val):
        # When BLE connection is being changed...
        # Auto update the "Connect" button properties
        self.btn_connect.setText('Disconnect' if val else 'Connect')
        self.btn_connect.clicked.disconnect()
        self.btn_connect.clicked.connect(self.handle_disconnect if val else self.handle_connect)
        
    @qasync.asyncSlot()
    async def change_state_selecting_ble(self):
        # When BLE device selection is being changed...
        # Auto update the "Connect" button availability only when user has at least select one item
        
        if self.list_devices.currentItem() is None or self.list_devices.currentRow() < 0:
            self.btn_connect.setEnabled(False)
            return
        
        self.btn_connect.setEnabled(True)
        
        # Continue if the user has selected a row
        selected_item = self.list_devices.selectedItems()
        
        if not selected_item:
            return
            
        # Get the selected BLE name and address
        selected_index = self.list_devices.row(selected_item[0])
        target_dev_name, addr = self.vars['devices_list'][selected_index]
        
        self.vars['current_dev_addr'] = addr
    
        # Get the system status update if the selected BLE is connected
        if addr in self.sensors and self.sensors[addr].is_connected:
            self.change_state_connecting_ble(True)
            await self.handle_manual_update()
        
        else:
            # Reset system status If the selected BLE is not connected
            self.change_state_connecting_ble(False)
            self.handle_clear_system_status()
            
    def change_state_checkbox_icm(self, mode, val):
        # When receiving state change from BLE
        # Only update UI widget if the state is not matched
        
        if mode == 0 and self.chkbox_raw_acc.isChecked != val:       # 0: Raw Acc
            self.chkbox_raw_acc.setChecked(val)
        elif mode == 1 and self.chkbox_cal_acc.isChecked != val:     # 1: Cal Acc
            self.chkbox_cal_acc.setChecked(val)
        elif mode == 2 and self.chkbox_lin_acc.isChecked != val:     # 2: Lin Acc
            self.chkbox_lin_acc.setChecked(val)
        elif mode == 3 and self.chkbox_raw_gyr.isChecked != val:     # 3: Raw Gyr
            self.chkbox_raw_gyr.setChecked(val)            
        elif mode == 4 and self.chkbox_cal_gyr.isChecked != val:     # 4: Cal Gyr
            self.chkbox_cal_gyr.setChecked(val)
        elif mode == 5 and self.chkbox_raw_mag.isChecked != val:     # 5: Raw Mag
            self.chkbox_raw_mag.setChecked(val)
        elif mode == 6 and self.chkbox_cal_mag.isChecked != val:     # 6: Cal Mag
            self.chkbox_cal_mag.setChecked(val)
        elif mode == 7 and self.chkbox_quat_vec.isChecked != val:     # 7: Quat Vec
            self.chkbox_quat_vec.setChecked(val)
        elif mode == 8 and self.chkbox_quat_mag.isChecked != val:     # 8: Quat Mag
            self.chkbox_quat_mag.setChecked(val)
        else:
            return
        
    def change_state_checkbox_emg(self, mode):
        # When receiving state change from BLE
        # Only update UI widget if the state is not matched
        
        if mode == 2 != self.chkbox_emg.checkState():           # Raw EMG + RMS
            self.chkbox_emg.setCheckState(QtCore.Qt.CheckState.Checked)
        elif mode == 1 != self.chkbox_emg.checkState():         # Raw EMG
            self.chkbox_emg.setCheckState(QtCore.Qt.CheckState.PartiallyChecked)
        elif mode == 0 != self.chkbox_emg.checkState():
            self.chkbox_emg.setCheckState(QtCore.Qt.CheckState.Unchecked)
        else:
            return
        
    @qasync.asyncSlot()
    async def clicked_checkbox_icm(self, mode):
        # User changed the checkbox state in UI
        # Send command to BLE to change state in device
        
        addr = self.vars['current_dev_addr']
        if not addr in self.sensors or not self.sensors[addr].is_connected:
            return
            
        if mode == 0:       # 0: Raw Acc
            widget = self.chkbox_raw_acc
        elif mode == 1:     # 1: Cal Acc
            widget = self.chkbox_cal_acc
        elif mode == 2:     # 2: Lin Acc
            widget = self.chkbox_lin_acc
        elif mode == 3:     # 3: Raw Gyr
            widget = self.chkbox_raw_gyr            
        elif mode == 4:     # 4: Cal Gyr
            widget = self.chkbox_cal_gyr
        elif mode == 5:     # 5: Raw Mag
            widget = self.chkbox_raw_mag
        elif mode == 6:     # 6: Cal Mag
            widget = self.chkbox_cal_mag
        elif mode == 7:     # 7: Quat Vec
            widget = self.chkbox_quat_vec
        elif mode == 8:     # 8: Quat Mag
            widget = self.chkbox_quat_mag
        else:
            return
        
        val = 1 if widget.isChecked() else 0
        
        # Change enable state at the BLE device
        await self.handle_send(f'A,W,{mode},{val}')
        
        # Update variable storing stste of device
        mode_str = self.sensors[addr].list_str_icm_mode[mode]
        self.sensors[addr].icm_mode[mode_str] = val
        
    @qasync.asyncSlot()
    async def clicked_checkbox_emg(self, state):
        # User changed the checkbox state in UI
        # Send command to BLE to change state in device
        
        addr = self.vars['current_dev_addr']
        if not addr in self.sensors or not self.sensors[addr].is_connected:
            return
        
        state = self.chkbox_emg.checkState()

        if state == QtCore.Qt.CheckState.Checked:
            val = 2
        elif state == QtCore.Qt.CheckState.PartiallyChecked:
            val = 1
        elif state == QtCore.Qt.CheckState.Unchecked:
            val = 0
        else:
            return
        
        # Change enable state at the BLE device
        await self.handle_send(f'A,w,{val}')
        
        # Update variable storing state of device
        self.sensors[addr].emg_mode = val
    
        
def main():
    # Load and shiw UI from QT Designer
    app = QtWidgets.QApplication(sys.argv)
    ui = UserInterface(path=os.path.join(os.path.dirname(__file__), 'EMGS.ui'))
    ui.show()

    # Set up and run event loop for asyncio
    loop = qasync.QEventLoop(app)
    asyncio.set_event_loop(loop)
    with loop:
        loop.run_forever()


if __name__ == '__main__':
    main()
