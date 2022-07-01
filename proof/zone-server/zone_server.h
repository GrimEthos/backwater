#pragma once

#include "zone_server_interfaces.h"



////////////////////////////////////////////////////////////

class ZoneServer
{
public:
                                            ZoneServer( );

    cpp::Time                               runFrame( );

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

inline cpp::Time ZoneServer::runFrame( )
{
    cpp::Duration frameDelta = cpp::Duration::ofMicros( 1000 * 1000 / m_data.fps );

    auto now = cpp::Time::now( );
    cpp::Duration clockDelta = now - m_data.time;
    int frames = (int)( clockDelta.micros( ) / frameDelta.micros( ) );

    if ( frames > 2 )
    {
        m_data.fps /= 2;
        frameDelta = cpp::Duration::ofMicros( 1000 * 1000 / m_data.fps );

        cpp::Log::info( "skipped %d frames, reducing fps to %d", frames, m_data.fps );

        m_data.time = now;
    }
    else
    {
        for ( int i = 0; i < frames; i++ )
        {
            for ( auto & sectorItr : m_data.sectors )
            {
                auto & sector = sectorItr.second;
                for ( auto & zoneItr : sector.zones )
                {
                    auto & zone = zoneItr.second;
                    zone.sim.update( frameDelta );
                }
            }

            m_data.time += frameDelta;
        }
    }

    return m_data.time + frameDelta;
}
