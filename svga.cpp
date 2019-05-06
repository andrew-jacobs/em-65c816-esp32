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
//==============================================================================

#include <Arduino.h>

#include "svga.h"


uint16_t        SVGA::line      = 0;

SVGA::SVGA (void)
{ }

void SVGA::begin (uint16_t hsync, uint16_t vsync, uint16_t signal)
{
    pinMode (SVGA::hsync = hsync, OUTPUT);
    pinMode (SVGA::hsync = vsync, OUTPUT);
    pinMode (SVGA::hsync = signal, OUTPUT);

    // SET up RMT
    // Interrupt on pin change
}

void IRAM_ATTR SVGA::onHSync (void)
{
    if (line < 599) {
        // Visible region (600)
        digitalWrite (vsync, LOW);

        // Start DMA
    }
    else if (line == 600) {
        // Front porch (1)
    }
    else if (line < 605) {
        // Sync pulse (4)
        digitalWrite (vsync, HIGH);
    }
    else {
        // Back porch (23)

    }

    if (++line == 628) line = 0;
}
