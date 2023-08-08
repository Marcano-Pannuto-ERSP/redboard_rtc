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
    time.sleep(0.5)

    ser.reset_input_buffer()
    ser.reset_output_buffer()

    # Gets the timestamps from the server
    constant_time(port, ser)

    # Checks the offset
    offset = server_test_time(port, 100, ser)

    # If offset is too big keep changing the time
    while math.fabs(offset) > 0.01:
        # Changes the time by the offset
        ser.write(bytearray(f"change_time {offset}\r\n", 'utf-8'))
        offset = server_test_time(port, 100, ser)

update_rtc("/dev/ttyUSB1")