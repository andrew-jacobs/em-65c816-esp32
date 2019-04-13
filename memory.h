

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
    const uint8_t      *pRd [RAM_BLOCKS + ROM_BLOCKS];
    uint8_t            *pWr [RAM_BLOCKS + ROM_BLOCKS];

    static uint32_t blockOf (uint32_t address);
    static uint32_t offsetOf (uint32_t address);

public:
    Memory ();

    Memory &add (uint32_t address, int32_t size);
    Memory &add (uint32_t address, uint8_t *pRAM, int32_t size);
    Memory &add (uint32_t address, const uint8_t *pROM, int32_t size);

    uint8_t getByte (uint32_t address);
    void setByte (uint32_t address, uint8_t value);
};

//------------------------------------------------------------------------------

inline uint32_t Memory::blockOf (uint32_t address)
{
    return ((address >> BLOCK_BITS) & (RAM_BLOCKS + ROM_BLOCKS - 1));
}

inline uint32_t Memory::offsetOf (uint32_t address)
{
    return (address & (BLOCK_SIZE - 1));
}

inline uint8_t Memory::getByte (uint32_t address)
{
    register const uint8_t *pBlock = pRd [blockOf (address)];

    return (pBlock [offsetOf (address)]);
}

inline void Memory::setByte (uint32_t address, uint8_t value)
{
    register uint8_t *pBlock = pWr [blockOf (address)];

    if (pBlock) pBlock [offsetOf (address)] = value;
}

#endif