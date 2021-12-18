#pragma once

#include <map>
#include <set>
#include "zone_interfaces.h"



namespace sector_server
{
    struct Data
    {
        using                               ViewServers = std::map<int, ISectorToViewServer::ptr_t>;
        using                               ZoneServers = std::map<int, ISectorToZoneServer::ptr_t>;

        SectorData                          sectorData;

        ViewServers                         viewServers;

        int                                 zoneServerIndex;
        ZoneServers                         zoneServers;
        std::map<cpp::XY<int>, int>         zoneToServerMap;
    };


    ////////////////////////////////////////////////////////////

    class FromView
        : public IViewToSectorServer
    {
    public:
        FromView( Data & data );

    private:
        Data & m_data;
    };


    ////////////////////////////////////////////////////////////

    class FromZone
        : public IZoneToSectorServer
    {
    public:
        FromZone( Data & data );

        virtual void                        addObject(
                                                const ObjectRef & object,
                                                const ZoneRef & zone,
                                                const ObjectData & objectData,
                                                std::function<void( Result, AddObjectReply )> result ) override;
        virtual void                        updateObject(
                                                const ObjectRef & object,
                                                const ZoneRef & zone,
                                                const ObjectData & objectData ) override;
        virtual void                        removeObject(
                                                const ObjectRef & object ) override;

    private:
        Data & m_data;
    };


    ////////////////////////////////////////////////////////////

    inline FromView::FromView( Data & data )
        : m_data( data )
    {

    }


    inline FromZone::FromZone( Data & data )
        : m_data( data )
    {

    }


    inline void FromZone::addObject(
        const ObjectRef & object,
        const ZoneRef & zone,
        const ObjectData & objectData,
        std::function<void( Result, AddObjectReply )> result )
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