# EM-65C816-ESP32
This project contains the source code for a 65C816 emulator designed for ESP32 based modules. It supports


## Memory

The ESP32 version of this emulator supports a 256K memory map split between RAM and ROM areas as shown in the following table. The ROM areas are mapped to ESP32 flash memory and are not writable at runtime.

Start    | End      | Description
-------- | -------- | ----------- 
$00:0000 | $00:efff | RAM
$00:f000 | $00:ffff | ROM (Boot)
$01:0000 | $01:ffff | RAM (Reserved for video)
$02:0000 | $02:ffff | RAM
$03:0000 | $03:ffff | RAM
$04:0000 | $04:ffff | ROM (OS)
$05:0000 | $05:ffff | ROM (Spare)
$06:0000 | $06:ffff | ROM (Spare)
$07:0000 | $07:ffff | ROM (Spare)

The 'rom' directory within the repository contains the tools and scripts needed to build the ROM image data files included into the emulator during compilation.

> At time of writing there is around 110K of spare heap memory at runtime so it would be possible to add another 64K RAM area instead of one of the ROM banks. The emulator is not especially large and memory map could be expanded to 512K with another 8 banks of ROM images.

> I found that the ESP32 RAM area was fragmented at startup and I could not allocated many large blocks dynamically. As a result most of the memory is allocated in 4K chunks

## Virtual Peripherals

Normally a 65C816 based computer would have a set of memory mapped peripherals but in an emulator the cost of checking very memory access adversely affects the speed of instruction execution so instead this emulator uses the WDM instruction ($42) to access a set of virtual peripherals.

Like BRK and COP the WDM instruction is followed by a 'signature' byte. The following table shows the currently supported values.

WDM # | Description
--- | -----------
$00 | Get IER value
$01 | Set IER value
$02 | Set bits in IER (IER |= C)
$03 | Clear bits in IER (IER &= ~C)
$04 | Get IFR value
$05 | Set IFR value
$06 | Set bits in IFR (IFR |= C)
$07 | Clear bits in IFR (IFR &= ~C)
$08 | Output A to Uart1
$09 | Input A from Uart1

Most of the operations use the full accumulator (C) or just its low byte (A). 

The following table shows how the bits in the 'Interrupt Enable Register' (IER) and 'Interrupt Flag Register' (IFR) are allocated to peripherals.

Bit # | Mask | Description
--- | ---- | ----------- 
0 | $0001 | Timer
1 | $0002 | Uart1 RX Full
2 | $0004 | Uart1 TX Empty 

See the boot ROM source code for examples of interrupt handlers that use the WDM functions.

## Observations

I'm a little disappointed with execution speed of the ESP32, especially considering that it has two cores. The best emulated CPU rate I have achieved is a little over 12MHz. As soon you use Arduino functions for the UART the performance dives.

## To Do:

These are all the bits and pieces I have yet to get around to:

- Video generation (800x600 monochrome). I think the ESP32 is capable of generating a mono-SVGA image. The RMT module can be used to generate the HSYNC pulse and trigger an interrupt routine that generates the VSYNC and starts DMA transfers to the I2S module to generate the pixels.  
- PS/2 Keyboard & Mouse interface. The biggest issue is the 5V <-> 3V3 level conversion here. Shouls like a job for some FETs.
- OS & Application ROMs. 