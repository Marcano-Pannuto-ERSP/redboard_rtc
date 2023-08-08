# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText 2023 Kristin Ebuengan
# SPDX-FileCopyrightText 2023 Melody Gill
# SPDX-FileCopyrightText 2023 Gabriel Marcano

import datetime
import serial
import numpy
import time

def server_test_time(port, trials):
    STATUS = trials // 5
    offsetAvg = []

    # Find timestamp for receiving request packet
    ser = serial.Serial(port) # open serial port
    ser.baudrate = 115200
    time.sleep(0.5) # this HAS to be here

    for x in range(trials):

        ser.reset_input_buffer()
        ser.reset_output_buffer()

        ser.write(bytearray("ping\r\n", 'utf-8'))

        # Finds the request
        line = ser.readline()
        print(line)
        while line.decode('utf-8')[0:-2] != "request":
            line = ser.readline()
        t1 = time.time()

        # Record timestamp of sending response packet
        ser.write(bytearray("response\r\n", 'utf-8'))
        t2 = time.time()

        # Record timestamps from pico
        picoStamps = ser.readline()

        # Gets milliseconds from timestamps
        picoSplit = str(picoStamps)[2:-5]
        picoLists = picoSplit.split(" ")

        t0Sec = int(picoLists[0])
        t0Mil = int(picoLists[1]) / 1000000
        t0 = t0Sec + t0Mil

        t3Sec = int(picoLists[2])
        t3Mil = int(picoLists[3]) / 1000000
        t3 = t3Sec + t3Mil

        # Get the offset
        firstHalf = t1 - t0
        secondHalf = t2 - t3
        finalResult = (firstHalf + secondHalf)/2
        offsetAvg.append(finalResult)

        # Keep track of trials
        if x % STATUS == 0:
            print(str(x) + " trials to check time accuracy completed")

    toReturn = sum(offsetAvg)/trials
    print("Average offset in seconds: " + str(toReturn))
    print("Standard Deviation: " + str(numpy.std(offsetAvg)))
    ser.close()
    return toReturn

# server_test_time("/dev/ttyUSB1", 100)
