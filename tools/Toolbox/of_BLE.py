import asyncio
from dataclasses import dataclass
from bleak import BleakScanner, BleakClient


@dataclass
class BLE:
    name: str = None
    uuid_dev: str = None
    uuid_read: str = None
    uuid_write: str = None
    uuid_notify: str = None
    uuid_desc: str = None
    client: BleakClient = None
    
    def run_asyncio(func):
        # Decorator to asyncio run any function
        def inner(self, *args, **kwargs):
            return asyncio.run(func(self, *args, **kwargs))
        
        return inner
    
    @run_asyncio
    async def scan(self):
        # Scan nearby BLE device
        
        print("\n>> Scan nearby BLE devices...\n")
        # Scan and list devices
        devices = await BleakScanner.discover()
        for device in devices:
            print(device)
            
    @run_asyncio    
    async def show_services(self, uuid=None):
        # Show target BLE device services and its characteristics
        
        if uuid is None:
            # Show services of selected UUID if available, otherwire terminate
            if self.uuid_dev is None:
                return
            uuid = self.uuid_dev
        
        async with BleakClient(uuid) as client:
            # Connect to the device
            if client.is_connected:
                print(f"\n>> Connected to Device UUID: {uuid}\n")
                
                # List characteristics of the device
                services = client.services
                for service in services:
                    print(f"\n>> Service UUID: {service.uuid}")
                    print(f"     Description: {service.description}")
                    for characteristic in service.characteristics:
                        characteristic.description
                        print(f"\n>>   Characteristic UUID: {characteristic.uuid}")
                        print(f"       Description: {characteristic.description}")
            else:
                print("\n>> Failed to connect to device.")
    
    async def connect(self):
        # Connect to a BLE device to read and write characteristics indefinitely
        
        if self.uuid_dev is None:
            # No BLE device is provided
            return
        
        self.client = BleakClient(self.uuid_dev)
        await self.client.connect()
                
        # Connect to the device
        if not self.client.is_connected:
            print(f"\n>> Device {self.name}: Failed to connect to the device UUID: {self.uuid_dev}")
            return
        
        print(f"\n>> Device {self.name}: Connected to UUID: {self.uuid_dev}")
        
        if self.uuid_notify is not None:
            # Start a task to receive notification from the characteristic indefinitely if available
            await self.client.start_notify(self.uuid_notify, self.handler_notify)
            print(f"\n>> Device {self.name}: Subscribed to notifications. Waiting for notifications...")
            
            await asyncio.sleep(1)
            
        if self.uuid_read is not None:
            while True:
                try:
                    # Start a loop to read values from characteristic if available
                    data = await self.client.read_gatt_char(self.uuid_read)
                    self.handler_read(self.client, data)
                except Exception as e:
                    print(f"\n>> Device {self.name}: Failed to read characteristic: {e}")
                    
                await asyncio.sleep(1)
                
    async def disconnect(self):
        
        if self.uuid_notify is not None:
            # Stop the task to receive notification from the characteristic if available
            await self.client.stop_notify(self.uuid_notify)
        
        self.handler_stop()     # Anything the program need to do to clean up
            
        # Disconnect to the device
        await self.client.disconnect()
            
    async def handler_write(self, data):
        await self.client.write_gatt_char(self.uuid_write, data)
    
    def handler_read(self, data):
        print(f"\n>> Device {self.name}: Read characteristic value from {self.client}: {data.decode('utf-8')}")
        
    def handler_notify(self, client, data):
        print(f"\n>> Device {self.name}: Received notification value from {self.client}: {data.decode('utf-8')}")
    
    def handler_stop(self):
        print(f">> Device {self.name}: Disconnect...")
        

if __name__ == '__main__':
    print(">> Begin...")

    ble = BLE()
    # ble.scan()
    ble.show_services(uuid='C81B5DAF-543E-D829-55F7-59B0E08C3E6F')
