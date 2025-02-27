import time          
import serial        

ser = serial.Serial(                 
        port='/dev/ttyAMA3',         
        baudrate=115200,                
        parity=serial.PARITY_NONE,       
        stopbits=serial.STOPBITS_ONE,    
        bytesize=serial.EIGHTBITS,        
        timeout=1                        
        )

print("connected to: " + ser.portstr)
while True:                         
    line=ser.readline()
    print(line)