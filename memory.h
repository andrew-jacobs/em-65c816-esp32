

#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

//==============================================================================

// Target Memory size (MUST be a power of 2)
#define RAM_SIZE            (256 * 1024)
#define ROM_SIZE            (256 * 1024)

// The size of a memory block (4K)
#define BLOCK_BITS          12
#define BLOCK_SIZE          (1 << BLOCK_BITS)

// The number of RAM and ROM blocks
#define RAM_BLOCKS          (RAM_SIZE / BLOCK_SIZE)
#define ROM_BLOCKS          (ROM_SIZE / BLOCK_SIZE)

//==============================================================================

class Memory
{
private:
    static const uint8_t  *pRd [RAM_BLOCKS + ROM_BLOCKS];
    static uint8_t        *pWr [RAM_BLOCKS + ROM_BLOCKS];

    Memory ();

    static uint32_t blockOf (uint32_t address)
    {
        return ((address >> BLOCK_BITS) & (RAM_BLOCKS + ROM_BLOCKS - 1));
    }

    static uint32_t offsetOf (uint32_t address)
    {
        return (address & (BLOCK_SIZE - 1));
    }

public:
    static void add (uint32_t address, int32_t size);
    static void add (uint32_t address, uint8_t *pRAM, int32_t size);
    static void add (uint32_t address, const uint8_t *pROM, int32_t size);

    static uint8_t getByte (uint32_t eal)
    {
        register const uint8_t *pBlock = pRd [blockOf (eal)];

        return (pBlock [offsetOf (eal)]);
    }

    static uint16_t getWord (uint32_t eal, uint32_t eah)
    {
        return (getByte (eal) | getByte (eah) << 8);
    }

    static void setByte (uint32_t eal, uint8_t value)
    {
        register uint8_t *pBlock = pWr [blockOf (eal)];

        if (pBlock) pBlock [offsetOf (eal)] = value;
    }
};
#endif