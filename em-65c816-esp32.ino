
#include <Arduino.h>

#include "memory.h"
#include "emulator.h"

//==============================================================================

uint8_t         video [64 * 1024];

const uint8_t   boot [4 * 1024] =
{
#include "fibonacci.h"
};

const uint8_t   code [256 * 1024] =
{
    0x00,
};

//==============================================================================

Memory      memory;
Emulator    emulator (memory);

void setup (void)
{
    Serial.begin (115200);

    Serial.println ();
    Serial.println ("EM-65C816-ESP32 [19.04]");
    Serial.println ("Copyright (C)2019 Andrew John Jacobs.");

    Serial.println ();
    Serial.println (">> Booting");

    Serial.println (">> Building memory description:");

    memory.add (0x000000, 0x00f000)                 // RAM (60K)
          .add (0x00f000, boot, sizeof(boot))       // ROM (4K)
          .add (0x010000, video, sizeof(video))     // RAM (64K)
          .add (0x020000, 0x020000)                 // RAM (128K)
          .add (0x040000, code, sizeof (code));     // ROM (256K)

    Serial.printf (">> Remaining Heap: %d\n", ESP.getFreeHeap ());
    Serial.println (">> Starting execution");
    emulator.reset ();
}

void loop (void)
{
    Serial.printf ("CYC=%d\n", emulator.step ());
}