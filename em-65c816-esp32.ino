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
#include "fifo.h"

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

Fifo<32> u1rx;
Fifo<32> u1tx;

// Signal timer interrupt at the configured rate
void doTimerTask (void *pArg)
{
    for (;;) {
        delay (1000 / CLK_FREQ);

        Emulator::ifr.tmr = 1;
    }
}

// Transfer Serial data into RX FIFO
void doU1rxTask (void *pArg)
{
    for (;;) {
        while (Serial.available () && !u1rx.isFull ())
            u1rx.enqueue (Serial.read ());
        delay (1);
    }
}

// Transfer Serial data out from TX FIFO
void doU1txTask (void *pArg)
{
    for (;;) {
        while (Serial.availableForWrite () && !u1tx.isEmpty ())
            Serial.write (u1tx.dequeue ());
        delay (1);
    }
}

void setup (void)
{
    Serial.begin (115200);
    Serial.printf (">> CPU running at %d MHz\n", ESP.getCpuFreqMHz ());

    Serial.println (">> Memory configuration:");
    Memory::add (0x000000, 0x00f000);                        // RAM (60K)
    Memory::add (0x00f000, boot, sizeof(boot));              // ROM (4K)
    Memory::add (0x010000, video.data, sizeof(video.data));  // RAM (64K)
    Memory::add (0x020000, 0x020000);                        // RAM (128K)
    Memory::add (0x040000, code, sizeof (code));             // ROM (256K)

    Serial.printf (">> Remaining Heap: %d\n", ESP.getFreeHeap ());
    Serial.println (">> Booting");

    xTaskCreatePinnedToCore (doTimerTask, "Timer", 1024, NULL, 1, &timerTask, 0);
    xTaskCreatePinnedToCore (doU1rxTask, "U1RX", 1024, NULL, 1, &u1rxTask, 0);
    xTaskCreatePinnedToCore (doU1txTask, "U1TX", 1024, NULL, 1, &u1txTask, 0);

    Emulator::reset ();

    cycles = 0;
    start = micros ();
}

void loop (void)
{
    register int        count = 1024;

    do {
       if (Emulator::isStopped ()) {
            delta = micros () - start;

            Serial.printf ("\n\nCycles = %d uSec = %d freq = ", cycles, delta);

            double speed = cycles / (delta * 1e-6);

            if (speed < 1000)
                Serial.printf ("%f Hz\n", speed);
            else if ((speed /= 1000) < 1000)
                Serial.printf ("%f kHz\n", speed);
            else
                Serial.printf ("%f MHz\n", speed / 1000);

            for (;;) delay (1000);
       }
       else {
            Emulator::ifr.u1rx = !u1rx.isEmpty ();
            Emulator::ifr.u1tx = !u1tx.isFull (); 

            cycles += Emulator::step ();
       }

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

    case 0x08:  c.w = ier.f & ifr.f; break;

    case 0x10:	{
            u1tx.enqueue (c.l);
            break;
        }
    case 0x11:	{
            c.l = u1rx.dequeue ();
            break;
        }
    }
    return (3);
}