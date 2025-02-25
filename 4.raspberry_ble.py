import asyncio
from bleak import BleakScanner, BleakClient
import logging

# Set up logging for debugging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Service and characteristic UUIDs for BT05 module
# Note: Actual UUIDs should be confirmed via datasheet or scanning
UART_SERVICE_UUID = "0000FFE0-0000-1000-8000-00805F9B34FB"  # BT05 UART service (common UUID)
UART_RX_CHAR_UUID = "0000FFE1-0000-1000-8000-00805F9B34FB"  # RX characteristic

# Buffer for accumulating received data packets
received_buffer = bytearray()

async def discover_bt05():
    """Scan for nearby BT05 modules."""
    logger.info("Scanning for Bluetooth devices...")
    devices = await BleakScanner.discover()
    
    # Filter devices by name
    bt05_devices = [d for d in devices if d.name and "BT05" in d.name]
    
    if not bt05_devices:
        logger.info("No BT05 devices found. All discovered devices:")
        for d in devices:
            logger.info(f"Device: {d.name} ({d.address})")
        return None
    
    logger.info(f"BT05 device found: {bt05_devices[0].name} ({bt05_devices[0].address})")
    return bt05_devices[0]

async def connect_to_bt05(device):
    """Connect to BT05 device and receive data."""
    global received_buffer
    
    if not device:
        logger.error("No device to connect to.")
        return
    
    logger.info(f"Connecting to {device.address}...")
    
    async with BleakClient(device.address) as client:
        logger.info(f"Connection status: {client.is_connected}")
        
        # Discover services (for debugging)
        services = await client.get_services()
        for service in services:
            logger.info(f"Service: {service.uuid}")
            for char in service.characteristics:
                logger.info(f"  Characteristic: {char.uuid}")
                logger.info(f"    Properties: {char.properties}")
        
        # Notification handler with buffering
        def notification_handler(sender, data):
            """Callback function for receiving data from BT05"""
            global received_buffer
            
            # Add received data to buffer
            received_buffer.extend(data)
            
            #logger.info(f"Received data chunk: {data.hex()}")
            
            # Check for termination character (e.g. newline)
            if b'\r\n' in received_buffer:
                # Extract complete messages
                messages = received_buffer.split(b'\r\n')
                for i in range(len(messages) - 1):
                    complete_message = messages[i]
                    try:
                        decoded_message = complete_message.decode('utf-8')
                        print(f"(hex): {complete_message.hex()}")
                        print(f"{decoded_message}")
                        print()
                    except UnicodeDecodeError:
                        logger.info(f"Complete message (hex): {complete_message.hex()}")
                
                # Keep the last incomplete message in the buffer
                received_buffer = messages[-1]
                #logger.info(f"Remaining in buffer: {received_buffer.hex()}")
        
        # Check if BT05 has the required service/characteristic
        if UART_SERVICE_UUID.lower() in [service.uuid.lower() for service in services]:
            char_found = False
            for service in services:
                if service.uuid.lower() == UART_SERVICE_UUID.lower():
                    for char in service.characteristics:
                        if char.uuid.lower() == UART_RX_CHAR_UUID.lower():
                            char_found = True
                            # Set up notifications
                            await client.start_notify(char.uuid, notification_handler)
                            logger.info(f"Started notifications for characteristic {char.uuid}")
            
            if not char_found:
                logger.error("Required characteristic not found.")
                return
            
            # Example of sending data to the device
            tx_char = None
            for service in services:
                if service.uuid.lower() == UART_SERVICE_UUID.lower():
                    for char in service.characteristics:
                        if "write" in char.properties:
                            tx_char = char.uuid
            
            if tx_char:
                # Send test data
                test_data = b"AT\r\n"  # Example AT command
                await client.write_gatt_char(tx_char, test_data)
                logger.info(f"Sent data: {test_data}")
            
            # Wait for data reception for 60 seconds
            logger.info("Waiting for data for 60 seconds...")
            await asyncio.sleep(60.0)
            
            # Stop notifications
            if char_found:
                await client.stop_notify(UART_RX_CHAR_UUID)
                logger.info("Stopped notifications.")
        else:
            logger.error(f"Required service {UART_SERVICE_UUID} not found.")

# Helper function to send long messages in chunks
async def send_long_message(client, char_uuid, message, chunk_size=20):
    """Send a long message by breaking it into chunks"""
    message_bytes = message.encode('utf-8') if isinstance(message, str) else message
    
    logger.info(f"Sending long message in chunks (total length: {len(message_bytes)} bytes)")
    
    for i in range(0, len(message_bytes), chunk_size):
        chunk = message_bytes[i:i+chunk_size]
        await client.write_gatt_char(char_uuid, chunk)
        logger.info(f"Sent chunk {i//chunk_size + 1}: {chunk}")
        # Add a small delay between chunks
        await asyncio.sleep(0.1)
    
    logger.info("Long message transmission complete")

async def main():
    """Main function"""
    # Reset buffer at the start
    global received_buffer
    received_buffer = bytearray()
    
    # Scan for BT05 device
    device = await discover_bt05()
    
    if device:
        # Connect to BT05 device
        await connect_to_bt05(device)
    else:
        logger.error("No BT05 device found.")

# Run the async event loop
if __name__ == "__main__":
    asyncio.run(main())