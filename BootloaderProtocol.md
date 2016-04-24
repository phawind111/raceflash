# Basic Operation #

The bootloader is located at 0x40000000-0x40007FFF (=32KB) of usable flash memory.
It emulates a usbser.sys Serial Port with 115.2k/8/N/1 and responds only to certain messages.
No output is happening until you 'wake' it.

# Steps #

  * Say Hello/Get Model type & serial number
  * Get the SEED from the device
  * Send the calculated key back to the device. (this really unlocks the bootloader flash application)
  * Erase the flash areas
  * Upload the new firmware to 0x40008000 onwards.
  * Save the FLASH-Key to 0x400C3FFC, without it the bootloader will not start the uploaded firmware
  * Close communication (reboot device)

# How to calculate the FLASH Key #

> The FLASH key is the 32 bit summary of all data in the ranges of 0x40008000-0x40003FFFF and 0x400C000-0x400C3FFFF.

&lt;BR&gt;


> and this sum needs to be a simple 0x0 (on its lower 32 bits ;-)

&lt;P&gt;



> As the FLASH key itself (0x400C3FFC = last 4 bytes of flash) is part of the FLASH you are best reading only till 0x400C3FFC

&lt;BR&gt;


> and substracting this sum from 0x00000000 to get to the last 4 bytes that you need to write. You can predo this calculation

&lt;BR&gt;


> before you start writing the firmware or as RL does it, you read all the data back in once you have written it, and then

&lt;BR&gt;


> calc and write the flash key on its own. (FLASH programming function will allow you to write only 4 bytes as well)

&lt;BR&gt;


> If the FLASH key is invalid, the device will stay in the bootldr mode.

## Say Hello/Get Model type& serial # ##

  * TX: 0xFF | 0x20 | 0x00 0x06 | CRC CRC |
  * RX: 0xFF | 0x01 | 0x00 0x16 | 0x65 0x72 0x4D 0x65 0x74 0x65 0x72 0xFF 0xFF 0xFF 0x2D | 0xDE 0xAD 0xBE 0xEF | CRC CRC |

  * start of message, always 0xFF
  * type of message, always 1 byte
  * length of message including CRC (payload + 0x6)
  * model type string (PerMeter...-)
  * serial number as 4bytes (no mine is not 0xDEADBEEF ;-)
  * CRC-CCITT (XModem) over length-2 bytes (except the CRC ;-) - nize calc http://www.lammertbies.nl/comm/info/crc-calculation.html



## Get the SEED from the device ##

  * TX: 0xFF | 0x12 | 0x00 0x06 | 0x06 0x66 |
  * RX: 0xFF | 0x01 | 0x00 0x08 | 0x9B 0x34 | 0xE5 0x89 |

  * 2 byte SEED value

## Send the calculated flash key ##

  * TX: 0xFF | 0x13 | 0x00 0x08 | 0x45 0xE4 | 0x58 0x05 |
  * RX: 0xFF | 0x01 | 0x00 0x06 | 0x1C 0x55 |

  * 2 byte KEY value
KEY Bytes = ( SEED Bytes ) CRC-CCIITT'ed with a poly value of 0x45DC
type of message 0x1 indicates that the unlock was successfull.


## Set diplay to erase flash mode (can be obmitted) ##

  * TX: 0xFF | 0x1B | 0x00 0x0A | 0x00 0x00 0x00 0x06| 0xCE 0x73 |
  * RX: 0xFF | 0x01 | 0x00 0x06 | | 0x1C 0x55 |

  * 0x000006 = probably some bytes that tell the device which screen to show

## Erase Flash Section 0-X (if u only want to read the device, you dont need to send it ##

  * TX: 0xFF | 0xE5 | 0x00 0x08 | 0x20 0x00 | 0x83 0x27 |
  * RX: 0xFF | 0x01 | 0x00 0x06 | 0x1C 0x55 |

  * sets the corresponding bits in the FLASH\_CR1 register
the RX message will happen when the actual erase has been done (1-2 secs) Repeat those message to clear all application flash areas (B0F5,B0F6,B0F7,B1F0,B1F1) by resending with values 0x10 0x00, 0x00 0x80, 0x00 0x40, 0x00 0x20, 0x00 0x10.



## Programm Flash 0x400 byte range ##

  * TX: 0xFF| 0x03 | 0x40 0x0A |  0x40 0x00 0x80 0x00 | 0x400 Data Bytes| CRC CRC |
  * RX: 0xFF| 0x01 | 0x00 0x06 | 0x1C 0x55 |

  * FLASH programming address (0x40008000)
  * Size of Program Data (0x400)
  * Program Data  (lots of bytes ....)


Next address will be 0x40008400, 0x40008800, ...
NOTE: There are two ranges: 0x4008000 - 0x4003FFF and 0x400C000 - 0x400C3FFF



## Read from FLASH (hint: any range, even the bootloader ;-) ##


  * TX: 0xFF | 0x04 | 0x00 0x0C | 0xAA 0xAA 0xAA 0xAA | 0x40 0x00 | CRC CRC |
  * RX: 0xFF | 0x01 | 0x00 0x06 | 0x1C 0x55 |

  * FLASH read address - e.g. 0x40 0x00 0x00 0x00
  * 0x400 = Number of bytes to read

## Close communication & reset device (doesnt work all the time, and not needed if you pull the plug manually) ##

  * TX: 0xFF | 0x50 | 0x00 0x06 | | CRC CRC |
  * RX: 0xFF | 0x01 | 0x00 0x06 | | 0x1C 0x55 |