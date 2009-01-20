
/* Copyright (c) 2008, Stefan Eilemann <eile@equalizergraphics.com> 
   All rights reserved. */

#include "view.h"

#include <eq/net/dataIStream.h>
#include <eq/net/dataOStream.h>

namespace eq
{
View::View()
        : _dirty( DIRTY_NONE )
        , _current( TYPE_NONE )
        , _eyeBase( 0.f )
{
}

View::View( net::DataIStream& is )
        : _eyeBase( 0.f )
{
    deserialize( is );
}

View::~View()
{
    _current = TYPE_NONE;
}

void View::getInstanceData( net::DataOStream& os )
{
    serialize( os, DIRTY_ALL );
}

void View::pack( net::DataOStream& os )
{
    if( _dirty == DIRTY_NONE )
        return;

    serialize( os, _dirty );
    _dirty = DIRTY_NONE;
}

void View::applyInstanceData( net::DataIStream& is )
{
    deserialize( is );
}

void View::serialize( net::DataOStream& os, const uint32_t dirtyBits )
{
    os << _current;
    if( _current == TYPE_NONE ) // OPT
        return;

    os << dirtyBits;
    if( dirtyBits & DIRTY_WALL )
        os << _wall;
    if( dirtyBits & DIRTY_PROJECTION )
        os << _projection;
    if( dirtyBits & DIRTY_EYEBASE )
        os << _eyeBase;
    if( dirtyBits & DIRTY_NAME )
        os << _name;
}

void View::deserialize( net::DataIStream& is )
{
    is >> _current;
    if( _current == TYPE_NONE ) // OPT
    {
        _dirty = DIRTY_NONE;
        return;
    }

    is >> _dirty;
    if( _dirty & DIRTY_WALL )
        is >> _wall;
    if( _dirty & DIRTY_PROJECTION )
        is >> _projection;
    if( _dirty & DIRTY_EYEBASE )
        is >> _eyeBase;
    if( _dirty & DIRTY_NAME )
        is >> _name;
}

void View::setWall( const Wall& wall )
{
    _wall       = wall;
    // TODO write '= wall' for Projection and update projection here
    _current    = TYPE_WALL;
    _dirty     |= DIRTY_WALL;
}

void View::setProjection( const Projection& projection )
{
    _projection = projection;
    _current    = TYPE_PROJECTION;
    _dirty     |= DIRTY_PROJECTION;
}

void View::setEyeBase( const float eyeBase )
{
    _eyeBase = eyeBase;
    _dirty  |= DIRTY_EYEBASE;
}

void View::setName( const std::string& name )
{
    _name = name;
    _dirty |= DIRTY_NAME;
}

const std::string& View::getName() const
{
    return _name;
}

std::ostream& operator << ( std::ostream& os, const View& view )
{
    switch( view.getCurrentType( ))
    {
        case View::TYPE_WALL:
            os << view.getWall();
            break;
        case View::TYPE_PROJECTION:
            os << view.getProjection();
            break;
        default: 
            os << "INVALID VIEW";
            break;
    }
    return os;
}
}
