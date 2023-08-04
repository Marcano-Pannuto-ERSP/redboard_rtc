# Redboard Artemis Initializing the RTC

This repository initializes an AM1815 RTC (sets time, sets alarm, set countdown timer, disables pins, etc)

Sets the time of the RTC with accuracy of 0.02 seconds

Initializes the RTC by:
- enabling/disabling trickle charging for backup battery
- disabling unused pins
- changing settings to specify disabling SPI in absence of VCC
- enabling/disabling automatic RC/XT oscillator switching according to user input
- configuring the RTC alarm
- configuring the timer to repeat at interval specified by user
- outputting alarm interrupts to pin FOUT/nIRQ
- outputting timer interrupts to pin PSW/nIRQ2
- writing to register 1 bit 7 to signal that this program initialized the RTC

## Dependencies
 - https://github.com/gemarcano/AmbiqSuiteSDK
 - https://github.com/gemarcano/asimple

In order for the libraries to be found, `pkgconf` must know where they are. The
special meson cross-file property `sys_root` is used for this, and the
`artemis` cross-file already has a shortcut for it-- it just needs a
variable to be overriden. To override a cross-file constant, you only need to
provide a second cross-file with that variable overriden. For example:

Contents of `my_cross`:
```
[constants]
prefix = '/home/gabriel/.local/redboard'
```

# Compiling and installing
```
mkdir build
cd build
# The `artemis` cross-file is assumed to be installed per recommendations from
# the `asimple` repository
meson setup --prefix [prefix-where-sdk-installed] --cross-file artemis --cross-file ../my_cross --buildtype release
meson install
```

# License

See the license file for details. In summary, this project is licensed
Apache-2.0, except for the bits copied from the Ambiq SDK, which is BSD
3-clause licensed.
