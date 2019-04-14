

#ifndef SVGA_H
#define SVGA_H

#include <stdint.h>

#define VIDEO_WIDTH     800
#define VIDEO_HEIGHT    600
#define PIXELS_PER_BYTE 8

//==============================================================================

union VideoRAM {
    union {
        struct {
            uint16_t            offset [VIDEO_HEIGHT];
            uint8_t             pixels [VIDEO_HEIGHT][VIDEO_WIDTH / PIXELS_PER_BYTE];
        };
        uint8_t             data [64 * 1024];
    };
};

//==============================================================================

class SVGA
{
private:
    static uint16_t     hsync;
    static uint16_t     vsync;
    static uint16_t     signal;

    static uint16_t     line;

    SVGA (void);

    static void         onHSync (void);

public:
    static void begin   (uint16_t hsync, uint16_t vsync, uint16_t signal);
};

#endif