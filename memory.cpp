//==============================================================================
//  _____ __  __        __  ____   ____ ___  _  __   
// | ____|  \/  |      / /_| ___| / ___( _ )/ |/ /_  
// |  _| | |\/| |_____| '_ \___ \| |   / _ \| | '_ \ 
// | |___| |  | |_____| (_) |__) | |__| (_) | | (_) |
// |_____|_|__|_|___ __\___/____/ \____\___/|_|\___/ 
// | ____/ ___||  _ \___ /___ \                      
// |  _| \___ \| |_) ||_ \ __) |                     
// | |___ ___) |  __/___) / __/                      
// |_____|____/|_|  |____/_____|                     
//
//------------------------------------------------------------------------------                                                   
// Copyright (C),2019 Andrew John Jacobs
// All rights reserved.
//
// This work is made available under the terms of the Creative Commons
// Attribution-NonCommercial-ShareAlike 4.0 International license. Open the
// following URL to see the details.
//
// http://creativecommons.org/licenses/by-nc-sa/4.0/
//------------------------------------------------------------------------------
// Notes:
//
// The ESP32's heap appears to be highly fragmented at startup and only a few
// large blocks can be allocated before malloc fails. Using a smaller block
// size allows more RAM to be utilised.
//==============================================================================

#include <Arduino.h>

#pragma GCC optimize ("-O4")

#include "memory.h"

const uint8_t  *Memory::pRd [RAM_BLOCKS + ROM_BLOCKS];
uint8_t        *Memory::pWr [RAM_BLOCKS + ROM_BLOCKS];

//==============================================================================

// Construct and initialise a Memory instance
Memory::Memory ()
{ }

// Build a RAM region from dynamically allocated blocks
void Memory::add (uint32_t address, int32_t size)
{
    Serial.printf ("%.6x-%.6x: RAM (Allocated)\n", address, address + size - 1);

    for (; size > 0; address += BLOCK_SIZE, size -= BLOCK_SIZE) {
        register uint32_t block = blockOf (address);
        register uint8_t *pRAM = (uint8_t *) malloc (BLOCK_SIZE);

        if (pRAM)
            pRd [block] = pWr [block] = pRAM;
        else
            Serial.printf ("!! Attempt to add NULL RAM block at %.6x", address);
    }
}

// Build a RAM region from an existing contiguous blocks
void Memory::add (uint32_t address, uint8_t *pRAM, int32_t size)
{
    Serial.printf ("%.6x-%.6x: RAM (Contiguous)\n", address, address + size - 1);
    
    if (pRAM) {
        for (; size > 0; address += BLOCK_SIZE, pRAM += BLOCK_SIZE, size -= BLOCK_SIZE) {
            register uint32_t block = blockOf (address);

            pRd [block] = pWr [block] = pRAM;
        }
    }
    else
        Serial.printf ("!! Attempt to add NULL RAM block at %.6x", address);
}

// Build a ROM region from an existing contiguous blocks
void Memory::add (uint32_t address, const uint8_t *pROM, int32_t size)
{
    Serial.printf ("%.6x-%.6x: ROM (Contiguous)\n", address, address + size - 1);

    if (pROM) {
        for (; size > 0; address += BLOCK_SIZE, pROM += BLOCK_SIZE, size -= BLOCK_SIZE) {
            register uint32_t block = blockOf (address);

            pRd [block] = pROM;
            pWr [block] = NULL;
        }
    }
    else
        Serial.printf ("!! Attempt to add NULL ROM block at %.6x", address);
}