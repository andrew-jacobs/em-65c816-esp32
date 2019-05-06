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

volatile uint32_t   cycles;
volatile uint32_t   start;
volatile uint32_t   delta;

TaskHandle_t        task;

void emulatorTask (void *pArg)
{
//    static uint32_t     count;

    for (;;) {
        while (Emulator::isStopped ()) {
            delay (100);
            Serial.print (".");
        }

        cycles = 0;
//        count = 10000000;
        start = micros ();

        while (!Emulator::isStopped ()) {
            cycles += Emulator::step ();
//
//            if (--count == 0) {
//                delay (1);
//                count = 10000000;
//            }
        }
        
        delta = micros () - start;
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
    xTaskCreatePinnedToCore (emulatorTask, "Emulator", 1024, NULL, 0, &task, 0);

    Emulator::reset ();
}

void loop (void)
{
    if (Emulator::isStopped ()) {
        Serial.printf ("\nCycles = %d uSec = %d freq = ", cycles, delta);

        double speed = cycles / (delta * 1e-6);

        if (speed < 1000)
            Serial.printf ("%f Hz\n", speed);
        else if ((speed /= 1000) < 1000)
            Serial.printf ("%f kHz\n", speed);
        else
            Serial.printf ("%f MHz\n", speed / 1000);
    
        delay (500);
        Emulator::reset ();
    }
}