# EM-65C816-ESP32
This project contains the source code for a 65C816 emulator designed for ESP32 based modules. It supports


## Memory

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


## Virtual Peripherals

Normally a 65C816 based computer would have a set of memory mapped peripherals but in an emulator the cost of checking very memory access adversely affects the speed of instruction execution so instead this emulator uses the WDM instruction ($42) to access a set of virtual peripherals.

Like BRK and COP the WDM instruction is followed by a 'signature' byte 

WDM # | Description
--- | -----------
$00 | Get IER
$01 | Set IER
$02 | Get IFR



## To Do:

