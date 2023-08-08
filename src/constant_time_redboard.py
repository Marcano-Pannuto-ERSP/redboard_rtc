import time
import serial

"""
write program on server to call redboard_rtc commands to do time sync
"""

INTERVAL = 0.05  # in seconds
DELAY = 0.05  # in seconds

def constant_time(port, ser):
    startTime = time.monotonic()
    endTime = startTime + INTERVAL
    now = time.monotonic()

    while now < endTime:   # runs for the specified interval
        currTime = time.time()
        currSeconds = int(currTime)
        currHundredths = int((currTime - currSeconds) * 1000000)

        ser.write(bytearray(f"set_time {currSeconds} {currHundredths}\r\n", 'utf-8'))

        time.sleep(DELAY)
        now = time.monotonic()

# constant_time("/dev/ttyUSB1")