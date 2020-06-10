import serial
# replace /dev/ttyACM0 with your serial usb port from the pi 
ser = serial.Serial('/dev/ttyACM0',115200)
while True:
	read_serial=ser.readline()
	print(read_serial)
