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

#pragma GCC optimize ("-O3")

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

TaskHandle_t    timerTask;
TaskHandle_t    u1rxTask;
TaskHandle_t    u1txTask;

uint32_t        cycles;
uint32_t        start;
uint32_t        delta;

volatile uint8_t u1rxd;
volatile uint8_t u1txd;

// Signal timer interrupt every 10 milliseconds
void doTimerTask (void *pArg)
{
    for (;;) {
        delay (100);

        Emulator::ifr.tmr = 1;
    }
}

// Generate a RXD interrupt if serial data is available
void doU1rxTask (void *pArg)
{
    for (;;) {
        while (!Serial.available ())
            delay (1);
        u1rxd = Serial.read ();
        Emulator::ifr.u1rx = 1;
        vTaskSuspend (NULL); 
    }
}

// Generate a TXD interrupt if serial transmission is possible
void doU1txTask (void *pArg)
{
    for (;;) {
        Emulator::ifr.u1tx = 1;
        vTaskSuspend (NULL);
        Serial.write (u1txd);
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

    xTaskCreate (doTimerTask, "Timer", 1024, NULL, 1, &timerTask);
    xTaskCreate (doU1rxTask, "U1RX", 1024, NULL, 1, &u1rxTask);
    xTaskCreate (doU1txTask, "U1TX", 1024, NULL, 1, &u1txTask);

    Emulator::reset ();

    cycles = 0;
    start = micros ();
}

void loop (void)
{
    register int        count = 1500;

    do {
       if (Emulator::isStopped ()) {
            delta = micros () - start;

            Serial.printf ("Cycles = %d uSec = %d freq = ", cycles, delta);

            double speed = cycles / (delta * 1e-6);

            if (speed < 1000)
                Serial.printf ("%f Hz\n", speed);
            else if ((speed /= 1000) < 1000)
                Serial.printf ("%f kHz\n", speed);
            else
                Serial.printf ("%f MHz\n", speed / 1000);

            for (;;) delay (1000);
       }
       else
            cycles += Emulator::step ();

    } while (--count);
}

uint8_t Common::op_wdm(uint32_t eal, uint32_t eah)
{
    TRACE(wdm);

    register uint8_t	cmnd = getByte(eal);

    switch (cmnd) {
    case 0x00:  c.w = ier.f;        break;
    case 0x01:  ier.f = c.w;        break;
    case 0x02:  ier.f |=  c.w;      break;
    case 0x03:  ier.f &= ~c.w;      break;

    case 0x04:  c.w = ifr.f;        break;
    case 0x05:  ifr.f = c.w;        break;
    case 0x06:  ifr.f |=  c.w;      break;
    case 0x07:  ifr.f &= ~c.w;      break;

    case 0x08:	{
            ifr.u1tx = 0;
            u1txd = c.l;
            vTaskResume (u1txTask);
            break;
        }
    case 0x09:	{
            ifr.u1rx = 0;
            c.l = u1rxd;
            vTaskResume (u1rxTask);
            break;
        }
    }
    return (3);
}
