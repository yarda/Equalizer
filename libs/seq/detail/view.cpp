
/* Copyright (c) 2011, Stefan Eilemann <eile@eyescale.ch> 
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 2.1 as published
 * by the Free Software Foundation.
 *  
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "view.h"

#include "config.h"
#include "pipe.h"

#include <seq/application.h>
#include <seq/renderer.h>
#include <seq/viewData.h>
#include <eq/client/config.h>
#include <eq/client/configEvent.h>

namespace seq
{
namespace detail
{

View::View( eq::Layout* parent )
        : eq::View( parent ) 
{}

View::~View()
{
}

Config* View::getConfig()
{
    return static_cast< Config* >( eq::View::getConfig( ));
}

Pipe* View::getPipe()
{
    return static_cast< Pipe* >( eq::View::getPipe( ));
}

ViewData* View::getViewData()
{
    return static_cast< ViewData* >( eq::View::getUserData( ));
}

const ViewData* View::getViewData() const
{
    return static_cast< const ViewData* >( eq::View::getUserData( ));
}

void View::notifyAttach()
{
    eq::View::notifyAttach();
    Pipe* pipe = getPipe();

    if( pipe ) // render client view
        setUserData( pipe->getRenderer()->createViewData( ));
    else // application view
        setUserData( getConfig()->getApplication()->createViewData( ));
}

void View::notifyDetached()
{
    ViewData* data = getViewData();
    setUserData( 0 );

    if( data )
    {
        Pipe* pipe = getPipe();

        if( pipe ) // render client view
            pipe->getRenderer()->destroyViewData( data );
        else // application view
            getConfig()->getApplication()->destroyViewData( data );
    }

    eq::View::notifyDetached();
}

bool View::updateData()
{
    if( !isActive( ))
        return false;

    ViewData* data = getViewData();
    EQASSERT( data );
    if( data )
        return data->update();
    return false;
}

bool View::handleEvent( const eq::ConfigEvent* event )
{
    ViewData* data = getViewData();
    EQASSERT( data );
    if( !data )
        return false;
    if( isActive( ))
        return data->handleEvent( event );
    data->handleEvent( event );
    return false;
}

}
}
