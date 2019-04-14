

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
