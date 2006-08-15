
/* Copyright (c) 2006, Stefan Eilemann <eile@equalizergraphics.com> 
   All rights reserved. */

#include "frameBuffer.h"

#include <eq/client/object.h>

using namespace eqs;

FrameBuffer::FrameBuffer()
        : eqNet::Object( eq::Object::TYPE_FRAMEBUFFER, eqNet::CMD_OBJECT_CUSTOM)
{
    setDistributedData( &_data, sizeof( eq::FrameBuffer::Data ));
}
