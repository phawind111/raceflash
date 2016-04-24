
```
Purpose:
========

With this software you are able to update a Performance box, with either
an original .RUF file or with an custom bin file. 
This software is interfacing the existing bootloader (Racelogic Updater) 
via an USB emulated serial port thats built into the device.


Features:
=========

	Update a PB with a .RUF file - overcome any update checks that the original updater.exe does 
	e.g. flash DriftBox image onto a PerformanceBox (tested) or vice vers (untested)
	Flash a custom .ihex file to the PerformanceBox via the bootloader, e.g. run your own application
	Calculate the SEED/Key pair used to authenticate with the bootloader
	Calculate the FLASH Key used to verify an software image (otherwise the bootloader will not run it)
        Create a .RUF file from a binary which can be used with "Upgrader.exe"

How to compile:
===============

make all

You need to have zlib1g-dev installed. apt-get is your friend.


Usage:
======

 raceflash [OPTIONS]

         -u -f <firmware.RUF> -d <serial_device>                - write <firmware.RUF> to flash
         -u -f <firmware.bin> -d <serial_device>                - write binary file to flash
         -c -f <firmware.RUF> -o <firmware1.bin>                - convert <firmware.RUF> to binary file
         -c -m <model> -f <firmware1.bin> -o <firmware.RUF>     - convert binary file to <firmware.RUF>

 <model> is 1 for PerformanceBox, 2 for DriftBox


Example1 - flashing a .RUF file
===============================

./raceflash -u -f /tmp/PB3.0.23.RUF -d /dev/ttyACM0

(SERIAL) Serial device '/dev/ttyACM0' opened
(RUF) .RUF file size: 238692 bytes
(RUF) Searching for model string ...
(RUF) Model: 'N00000000' found
(RUF) Searching for firmware(ihex) string ...
(RUF) Firmware found, size=680757
(RUF) Searching for 4096 bytes padding string ...
(RUF) Padding found, size=4096
(FLASH) Found device 'PerMeter' with serial# '501XXXX'
(IHEX) Extracted 227628 bytes from ihex data (0x40008000-0x4003FFFF)
(IHEX) Extracted 14324 bytes from ihex data (0x400C0000-0x400C3FFF)
(FLASH) Bootloader succesfully unlocked
(FLASH) Calculated FLASH Key=0xE069288C
(FLASH) Erasing sector 0x0200 - done
(FLASH) Erasing sector 0x0100 - done
(FLASH) Erasing sector 0x0080 - done
(FLASH) Erasing sector 0x0040 - done
(FLASH) Erasing sector 0x0020 - done
(FLASH) Erasing sector 0x0010 - done
(FLASH) programming (0x40008000 - 0x4003F92C) [*******************************************************************************************************************************************************************************************************************************] - successfully programmed
(FLASH) Last chunk was at 0x4003F800
(FLASH) programming (0x400C0000 - 0x400C37F4) [**************] - successfully programmed
(FLASH) Last chunk was at 0x400C3400
(FLASH) Writing FLASH Key=0xE069288C - done
(FLASH) Reseting device
```