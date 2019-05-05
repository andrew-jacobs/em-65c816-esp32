
#include <Arduino.h>

#pragma GCC optimize ("-O4")

#include "svga.h"
#include "memory.h"
#include "emulator.h"

//==============================================================================

const uint8_t   boot [4 * 1024] =
{
#include "boot.h"
};

const uint8_t   code [256 * 1024] =
{
#include "rom0.h"
#include "rom1.h"
#include "rom2.h"
#include "rom3.h"
};

//==============================================================================

VideoRAM    video;

void setup (void)
{
    Serial.begin (115200);

    Serial.println ();
    Serial.println ("EM-65C816-ESP32 [19.05]");
    Serial.println ("Copyright (C)2019 Andrew John Jacobs.");
    Serial.println ();

    Serial.printf (">> CPU running at %d MHz\n", ESP.getCpuFreqMHz ());

    Serial.println (">> Memory configuration:");
    Memory::add (0x000000, 0x00f000);                        // RAM (60K)
    Memory::add (0x00f000, boot, sizeof(boot));              // ROM (4K)
    Memory::add (0x010000, video.data, sizeof(video.data));  // RAM (64K)
    Memory::add (0x020000, 0x020000);                        // RAM (128K)
    Memory::add (0x040000, code, sizeof (code));             // ROM (256K)

    Serial.printf (">> Remaining Heap: %d\n", ESP.getFreeHeap ());
    Serial.println (">> Booting");
    
    Emulator::reset ();
}

void loop (void)
{
    //Serial.printf ("CYC=%d\n", emulator.step ());
    for (;;) Emulator::cycles += Emulator::step ();
    //delay (100);
}
