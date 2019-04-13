

#include <Arduino.h>

#include "svga.h"


uint16_t        SVGA::line      = 0;

SVGA::SVGA (void)
{ }

void SVGA::begin (uint16_t hsync, uint16_t vsync, uint16_t signal)
{
    pinMode (hysnc, OUTPUT);
    pinMode (vsync, OUTPUT);
    pinMode (signal, OUTPUT);


}

void IRAM_ATTR SVGA::onHSync (void)
{
    switch (line) {
    case 0..599:    // Visible region (600)
        digitalWrite (vsync, LOW);
        break;
    case 600..600:  // Front porch (1)
        break;
    case 601..604:  // Sync pulse (4)
        digitalWrite (vsync, HIGH);
        break;
    case 604..627   // Back porch (23)

    }

    if (++line == 628) line = 0;
}
