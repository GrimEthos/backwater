#pragma once

#include "sector_server_detail.h"



////////////////////////////////////////////////////////////

class SectorServer
{
public:
    SectorServer( );

    sector_server::FromView                 fromView;
    sector_server::FromZone                 fromZone;

private:
    sector_server::Data                     m_data;
};


////////////////////////////////////////////////////////////

inline SectorServer::SectorServer( )
    : fromView( m_data ), fromZone( m_data )
{

}
