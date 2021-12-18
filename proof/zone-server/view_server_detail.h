#pragma once

#include <map>
#include <set>
#include "zone_interfaces.h"



namespace view_server
{
    struct Data
    {
        ViewData                                viewData;
    };


    ////////////////////////////////////////////////////////////

    class FromSector
        : public ISectorToViewServer
    {
    public:
        FromSector( Data & data );

    private:
        Data & m_data;
    };


    ////////////////////////////////////////////////////////////

    class FromZone
        : public IZoneToViewServer
    {
    public:
        FromZone( Data & data );

        void                                updateObject(
                                                const ObjectRef & object,
                                                const ZoneRef & zone,
                                                const ObjectData & objectData ) override;
        void                                removeObject(
                                                const ObjectRef & object ) override;

    private:
        Data & m_data;
    };


    ////////////////////////////////////////////////////////////

    inline FromSector::FromSector( Data & data )
        : m_data( data )
    {

    }


    inline FromZone::FromZone( Data & data )
        : m_data( data )
    {

    }


    inline void FromZone::updateObject(
        const ObjectRef & object,
        const ZoneRef & zone,
        const ObjectData & objectData )
    {

    }


    inline void FromZone::removeObject(
        const ObjectRef & object )
    {

    }
}
