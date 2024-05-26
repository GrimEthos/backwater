#include <cpp/Time.h>

#include "sim.h"



namespace zone_server
{
    struct Sim::Detail
    {
        uint32_t                            lastFrame = 0;
        uint32_t                            timestamp = 0;
        int                                 fps = 64;
    };


    Sim::Sim( )
        : m_detail( std::make_shared<Detail>( ) )
    {

    }


    void Sim::update( cpp::Duration deltaTime )
    {
        uint32_t deltaMicros = deltaTime.micros( );
        uint32_t frameDelta = 1000000 / m_detail->fps;

        while ( m_detail->timestamp + deltaMicros > m_detail->lastFrame + frameDelta )
        {
            // run one frame
            printf( "." );

            m_detail->timestamp += frameDelta;
            m_detail->lastFrame = m_detail->timestamp;
            deltaMicros -= frameDelta;
        }

        m_detail->timestamp += deltaMicros;
    }
}

