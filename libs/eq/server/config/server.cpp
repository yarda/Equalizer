
/* Copyright (c) 2011, Stefan Eilemann <eile@eyescale.h> 
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

#include "server.h"

#include "display.h"
#include "resources.h"

#include "../config.h"
#include "../global.h"
#include "../loader.h"
#include "../server.h"

namespace eq
{
namespace server
{
namespace config
{

ServerPtr Server::configure( const std::string& session )
{
    Global::instance()->setConfigFAttribute( Config::FATTR_VERSION, 1.2f );
    ServerPtr server = new server::Server;

    Config* config = new Config( server );
    config->setName( session + " autoconfig" );

    if( !Resources::discover( config, session ))
        return 0;

    if( config->getNodes().size() > 1 )
        // add server connection for cluster configs
        server->addConnectionDescription( new ConnectionDescription );

    Display::discoverLocal( config );
    const Compounds compounds = Loader::addOutputCompounds( server );
    if( compounds.empty( ))
        return 0;

    const Channels channels = Resources::configureSourceChannels( config );
    Resources::configure( compounds, channels );
    return server;
}

}
}
}
