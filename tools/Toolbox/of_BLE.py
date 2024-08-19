import asyncio
from dataclasses import dataclass
from bleak import BleakScanner, BleakClient


@dataclass
class BLE:
    uuid_dev: str = None
    uuid_read: str = None
    uuid_write: str = None
    uuid_notify: str = None
    uuid_desc: str = None
    
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
    
    @run_asyncio    
    async def mainloop(self):
        # Connect to a BLE device to read and write characteristics indefinitely
        
        if self.uuid_dev is None:
            # No BLE device is provided
            return
        
        async with BleakClient(self.uuid_dev) as client:
            
            # Connect to the device
            if not client.is_connected:
                print(f"\n>> Failed to connect to the device UUID: {self.uuid_dev}\n")
                return
            print(f"\n>> Connected to Device UUID: {self.uuid_dev}\n")
            
            if self.uuid_notify is not None:
                # Start a task to receive notification from the characteristic indefinitely
                await client.start_notify(self.uuid_notify, self.handler_notify)
                print(">> Subscribed to notifications. Waiting for notifications...")
                await asyncio.sleep(1)
                
            async_task_write = asyncio.Task
            if self.uuid_write is not None:
                # Start a task to run the UI event loop
                async_task_write = asyncio.create_task(self.handler_write(client))
                print(">> Subscribed to write. User customize how to write...")
                
                await asyncio.gather(async_task_write)
                
            if self.uuid_notify is not None:
                self.handler_stop()
                await client.stop_notify(self.uuid_notify)
                    
    def handler_notifiy(self, client, data):
        pass
    
    def handler_stop(self):
        pass
    
    async def handler_write(self, client):
        pass
        

if __name__ == '__main__':
    print(">> Begin...")

    ble = BLE()
    ble.scan()
    ble.show_services()
