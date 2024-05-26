#pragma once

#include "view_server_detail.h"



////////////////////////////////////////////////////////////

class ViewServer
{
public:
    ViewServer( );

    view_server::FromSector                 fromSector;
    view_server::FromZone                   fromZone;

private:
    view_server::Data                       m_data;
};


////////////////////////////////////////////////////////////

inline ViewServer::ViewServer( )
    : fromSector( m_data ), fromZone( m_data )
{

}
