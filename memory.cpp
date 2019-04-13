
// The ESP32's heap appears to be highly fragmented at startup and only a few
// large blocks can be allocated before malloc fails. Using a smaller block
// size allows more RAM to be utilised.


#include <Arduino.h>

#include "memory.h"

//==============================================================================

// Construct and initialise a Memory instance
Memory::Memory ()
{
    for (uint32_t index = 0; index < (RAM_BLOCKS + ROM_BLOCKS); ++index)
        pRd [index] = pWr [index] = NULL;
}

// Build a RAM region from dynamically allocated blocks
Memory &Memory::add (uint32_t address, int32_t size)
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

    return (*this);
}

// Build a RAM region from an existing contiguous blocks
Memory &Memory::add (uint32_t address, uint8_t *pRAM, int32_t size)
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

    return (*this);
}

// Build a ROM region from an existing contiguous blocks
Memory &Memory::add (uint32_t address, const uint8_t *pROM, int32_t size)
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

    return (*this);
}