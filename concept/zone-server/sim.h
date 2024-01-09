#pragma once

#include "zone_interfaces.h"



namespace zone_server
{
    class Sim
    {
    public:
        Sim( );

        void update( cpp::Duration deltaTime );

    private:
        struct Detail;
        std::shared_ptr<Detail> m_detail;
    };
}
