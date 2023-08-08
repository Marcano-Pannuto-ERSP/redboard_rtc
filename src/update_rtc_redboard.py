# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText 2023 Kristin Ebuengan
# SPDX-FileCopyrightText 2023 Melody Gill
# SPDX-FileCopyrightText 2023 Gabriel Marcano

"""
Sets the time of the RTC with accuracy of 2 hundredths of seconds
"""

import math
import argparse
import serial
import time
from constant_time_redboard import constant_time
from server_test_time_redboard import server_test_time

def update_rtc(port):
    ser = serial.Serial(port) # open serial port
    ser.baudrate = 115200
    print("waiting")
    time.sleep(5)
    print("after waiting")

    ser.reset_input_buffer()
    ser.reset_output_buffer()

    # Gets the timestamps from the server
    constant_time(port)

    # Checks the offset
    offset = server_test_time(port, 100)
    print("after offset")

    # If offset is too big keep changing the time
    while math.fabs(offset) > 0.02:
        # Changes the time by the offset
        ser.write(bytearray(f"change_time {offset}\r\n", 'utf-8'))
        print("after change time")
        offset = server_test_time(port, 100)

update_rtc("/dev/ttyUSB1")