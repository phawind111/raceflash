  * CPU

> STR712FR2 - ARM7TDMI. 16/32-BIT MCU with FLASH, USB, CAN, 5 TIMERS, ADC, 10 COMMUNICATIONS INTERFACES
> STMicro Documentation

  * FLASH/RAM

> 64KB RAM
> 272KB Flash - 32KB used by RL Bootloader, 240KB for an OS without touching the RL bootloader
> RL seems to use the upper 16KB for images and/or graphics

  * JTAG

> Standard 10x2 Header for ARM JTAG. Tested to work with Amontec JTAG Tiny + OpenOCD

  * USB

> USB Device Port connected to STR712FR

  * Serial Port (Backside)

> Connected to STR712FR USART1 via MAX232 level shifter
> The MAX232 can handle up to two ports, its unknown if the 3rd USART is also connected there.

  * GPS Module

> Connected to STR712FR USART0
> uBLOX TIM 4-A-0 receiver. aka ANTARIS 4 module
> TIM-4 Datasheet

  * LCD

> Connected to STR712FR via serial mode (SPI like)
> Display controller is an ST7565S like device.
> Display is mounted upside/down.
> Backlight controlled via PWM from STR712FR.

  * Keyboard

> Connected to STR712FR via GPIO pins

  * SPI EEPROM

  1. 8Kbit (32KB) EEPROM connected to BSPI1 on STR712FR

  * SD CardReader

> Connected to BSPI0 on STR712FR

  * Clocks

  1. 16MHz CPU Crystal - PLL'ed to 24MHZ
  1. 4MHZ USB Crystal - PLL'ed to 48MHZ

  * Buzzer

> Probably PWM controlled, unknown as of know.