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
//-----------------------------------------------------------------------------
// Copyright (C)2018-2019 Andrew John Jacobs
// All rights reserved.
//
// This work is made available under the terms of the Creative Commons
// Attribution-NonCommercial-ShareAlike 4.0 International license. Open the
// following URL to see the details.
//
// http://creativecommons.org/licenses/by-nc-sa/4.0/
//==============================================================================
//
// Notes:
//
//------------------------------------------------------------------------------

#include <Arduino.h>

#pragma GCC optimize ("-O4")

#include "svga.h"
#include "memory.h"
#include "emulator.h"

//==============================================================================

// 4K Boot ROM image
const uint8_t   boot [4 * 1024] =
{
#include "boot.h"
};

// 256K OS/Application ROM images
const uint8_t   code [256 * 1024] =
{
#include "rom0.h"
#include "rom1.h"
#include "rom2.h"
#include "rom3.h"
};

//==============================================================================

VideoRAM        video;

TaskHandle_t    task;

void emulatorTask (void *pArg)
{
    uint32_t   cycles;
    uint32_t   start;
    uint32_t   delta;
//    uint32_t   count;

    for (;;) {
        Emulator::reset ();

        cycles = 0;
        start = micros ();

        while (!Emulator::isStopped ())
            cycles += Emulator::step ();

        delta = micros () - start;

        Serial.printf ("Cycles = %d uSec = %d freq = ", cycles, delta);

        double speed = cycles / (delta * 1e-6);

        if (speed < 1000)
            Serial.printf ("%f Hz\n", speed);
        else if ((speed /= 1000) < 1000)
            Serial.printf ("%f kHz\n", speed);
        else
            Serial.printf ("%f MHz\n", speed / 1000);

        //delay (100);
    }
}

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

    disableCore0WDT();
    xTaskCreatePinnedToCore (emulatorTask, "Emulator", 4096, NULL, 1, &task, 0);
}

void loop (void)
{
    ;
}