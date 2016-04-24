Extract DriftBox & PerformanceBox RUF files.
It has not been tested with other firmware images, feel free to try it out.

## How to compile: ##


make all

You need to have zlib1g-dev installed. apt-get is your friend.

## How to use: ##


./ruf\_extract FIRMWARE.RUF

Usually 3 files will be dumped:
  * chunk\_21.dat - 9 Bytes Long
> > contains a string C00000000 (DriftBox) or N00000000 (PerformanceBox)
  * chunk\_30.dat - XXXX Bytes Long - contains an Intel HEX File, obviously the firmware. Checkout "strings chunk\_30.dat"
  * chunk\_XXXX.dat - Where X depends on the size of chunk\_30.dat - contains 4096 bytes of 0x0



## RUF COMBINE HOWTO (create firmware image) ##

./ruf\_combine filename.RUF header chunk21.dat chunk\_30.dat pad\_4096.dat tail

will reassemble filename.RUF from the parts given, when u play with the IHEX file, make sure it contains DOS linebreaks (\r\n) (unix2dos helps) - as this is IHEX file convention ! (otherwise you want be able to flash it)

BTW try using an older Driftbox Firmware (2056) with the "C00000000" chunk21.dat - it will run even on the PerformanceBox (but no Gyro ;-)
They seem to have fixed that in the newer releases, but as the model type is in the flash ... well ;-)