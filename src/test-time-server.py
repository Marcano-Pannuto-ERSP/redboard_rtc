

# from datetime import datetime
import time
import serial

"""
write program on server to call redboard_rtc commands to do time sync
"""


INTERVAL = 0.05  # in seconds
DELAY = 0.05  # in seconds

def constant_time(port):
    ser = serial.Serial(port) # open serial port
    ser.baudrate = 115200

    startTime = time.monotonic()
    endTime = startTime + INTERVAL
    now = time.monotonic()

    while now < endTime:   # runs for the specified interval
        currTime = time.time()
        currSeconds = int(currTime)
        currHundredths = int((currTime - currSeconds) * 100)


        # currTime = datetime.utcnow()
        # currWeekday = time.gmtime().tm_wday
        # currMillisec = int(currTime.microsecond/10000)
        # currTuple = (
        #     currTime.year,
        #     currTime.month,
        #     currTime.day,
        #     currWeekday,
        #     currTime.hour,
        #     currTime.minute,
        #     currTime.second,
        #     currMillisec
        # )
        toWrite = bytearray(f"{currSeconds} {currHundredths}\n", 'utf-8')
        ser.write(toWrite)

        time.sleep(DELAY)
        now = time.monotonic()

    ser.close() # close serial port

constant_time("/dev/ttyUSB0")