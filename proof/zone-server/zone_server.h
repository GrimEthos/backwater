#pragma once

#include "zone_server_detail.h"



////////////////////////////////////////////////////////////

class ZoneServer
{
public:
    ZoneServer( );

    zone_server::FromSector                 fromSector;
    zone_server::FromView                   fromView;
    zone_server::FromZone                   fromZone;

private:
    zone_server::Data                       m_data;
};


////////////////////////////////////////////////////////////

inline ZoneServer::ZoneServer( )
    : fromSector( m_data ), fromView( m_data ), fromZone( m_data )
{

}
