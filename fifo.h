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

#ifndef FIFO_H
#define FIFO_H

#include <stdint.h>

//==============================================================================

template <uint16_t size> class Fifo
{
private:
    volatile uint16_t   head;
    volatile uint16_t   tail;
    volatile uint8_t    data [size];

public:
    // Construct an empty Fifo instance
    Fifo (void)
        : head(0), tail(0)
    { }

    // Is the Fifo completely full?
    bool isFull (void) const
    {
        return ((tail + 1) % size == head);
    }

    // Is the Fifo completely empty?
    bool isEmpty (void) const
    {
        return (head == tail);
    }

    // Enqueue a value. The Fifo MUST NOT be full.
    void enqueue (uint8_t value)
    {
        data [tail] = value;
        tail = (tail + 1) % size;
    }

    // Dequeue a value. The Fifo MUST NOT be empty
    uint8_t dequeue (void)
    {
        uint8_t value = data [head];
        head = (head + 1) % size;
        return (value);
    }
 };
#endif