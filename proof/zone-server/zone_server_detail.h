#pragma once

#include <map>
#include <set>
#include "zone_interfaces.h"



namespace zone_server
{
    struct Data
    {
        struct ZoneMeta
        {
            IZoneToZoneServer::ptr_t            izone;
            ZoneData                            zoneData;
        };
        struct SectorMeta
        {
            IZoneToSectorServer::ptr_t          isector;
            SectorData                          sectorData;
            std::map<cpp::XY<int>, ZoneMeta>    zones;
            std::set<cpp::XY<int>>              active;
        };
        struct ViewMeta
        {
            IZoneToViewServer::ptr_t            iview;
            ViewData                            viewData;
        };
        std::map<int, SectorMeta>               m_sectors;
        std::map<int, ViewMeta>                 m_views;
    };


    ////////////////////////////////////////////////////////////

    class FromSector
        : public ISectorToZoneServer
    {
    public:
                                            FromSector( Data & data );

        void                                updateSectorInfo(
                                                const SectorRef & sector,
                                                IZoneToSectorServer::ptr_t isector,
                                                const SectorData & sectorData ) override;
        void                                removeSectorInfo(
                                                SectorRef & sector ) override;
                                            
        void                                updateZoneInfo(
                                                const ZoneRef zone,
                                                IZoneToZoneServer::ptr_t izone,
                                                const ZoneData & zoneData ) override;
        void                                removeZoneInfo(
                                                const ZoneRef zone ) override;
                                            
        void                                updateViewInfo(
                                                const ViewRef & view,
                                                IZoneToViewServer::ptr_t iview,
                                                const ViewData & viewData ) override;
        void                                removeViewInfo(
                                                const ViewRef & view ) override;
                                            
        void                                updateObject(
                                                const ObjectRef & object,
                                                const ZoneRef & zone,
                                                const ObjectData & objectData ) override;
        void                                removeObject(
                                                const ObjectRef & object ) override;

    private:
        Data &                              m_data;
    };


    ////////////////////////////////////////////////////////////

    class FromView
        : public IViewToZoneServer
    {
    public:
                                            FromView( Data & data );

        void                                updateWatch(
                                                const ObjectRef & object,
                                                float distance ) override;
        void                                removeWatch(
                                                const ObjectRef & object ) override;
        void                                controlObject(
                                                const ObjectRef & object,
                                                uint32_t timestamp,
                                                int inputCount,
                                                int inputs[],
                                                float values[] ) override;
    private:
        Data &                              m_data;
    };


    ////////////////////////////////////////////////////////////

    class FromZone
        : public IZoneToZoneServer
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
        Data &                              m_data;
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
