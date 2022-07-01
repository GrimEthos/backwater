#pragma once

#include <map>
#include <set>
#include "zone_interfaces.h"
#include "zone_server_data.h"



namespace zone_server
{
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
                                                const ObjectData & objectData ) override;
        void                                removeObject(
                                                const SectorRef & sector,
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
                                                const ObjectData & objectData ) override;
        void                                removeObject(
                                                const SectorRef & sector,
                                                const ObjectRef & object ) override;

    private:
        Data &                              m_data;
    };


    ////////////////////////////////////////////////////////////

    inline FromSector::FromSector( Data & data )
        : m_data( data )
    {

    }


    inline void FromSector::updateSectorInfo(
        const SectorRef & sector,
        IZoneToSectorServer::ptr_t isector,
        const SectorData & sectorData )
    {
        auto & sectorInfo = m_data.sectors[sector.sectorId];
        sectorInfo.isector = isector;
        sectorInfo.sectorData = sectorData;
    }


    inline void FromSector::removeSectorInfo(
        SectorRef & sector )
    {
        m_data.sectors.erase( sector.sectorId );
    }


    inline void FromSector::updateZoneInfo(
        const ZoneRef zone,
        IZoneToZoneServer::ptr_t izone,
        const ZoneData & zoneData )
    {
        auto & sectorInfo = m_data.sectors[zone.sectorId];
        auto & zoneInfo = sectorInfo.zones[zone.zoneId];
        zoneInfo.izone = izone;
        zoneInfo.zoneData = zoneData;
        //zoneInfo.sim.reset();
    }


    inline void FromSector::removeZoneInfo(
        const ZoneRef zone )
    {
        auto & sectorInfo = m_data.sectors[zone.sectorId];
        sectorInfo.zones.erase( zone.zoneId );
    }


    inline void FromSector::updateViewInfo(
        const ViewRef & view,
        IZoneToViewServer::ptr_t iview,
        const ViewData & viewData )
    {
        auto & viewInfo = m_data.views[view.viewId];
        viewInfo.iview = iview;
        viewInfo.viewData = viewData;
    }


    inline void FromSector::removeViewInfo(
        const ViewRef & view )
    {
        m_data.views.erase( view.viewId );
    }


    inline void FromSector::updateObject(
        const ObjectRef & object,
        const ObjectData & objectData )
    {
        auto & sectorInfo = m_data.sectors[objectData.location.sectorId];
        bool isNew = sectorInfo.objects.insert_or_assign( object.objectId, objectData ).second;
    }


    inline void FromSector::removeObject(
        const SectorRef & sector,
        const ObjectRef & object )
    {
        auto & sectorInfo = m_data.sectors[sector.sectorId];
        sectorInfo.objects.erase( object.objectId );
    }


    ////////////////////////////////////////////////////////////

    inline FromView::FromView( Data & data )
        : m_data( data )
    {
    }


    void FromView::updateWatch(
        const ObjectRef & object,
        float distance )
    {
    }


    void FromView::removeWatch(
        const ObjectRef & object )
    {
    }


    void FromView::controlObject(
        const ObjectRef & object,
        uint32_t timestamp,
        int inputCount,
        int inputs[],
        float values[] )
    {
    }


    ////////////////////////////////////////////////////////////

    inline FromZone::FromZone( Data & data )
        : m_data( data )
    {

    }


    inline void FromZone::updateObject(
        const ObjectRef & object,
        const ObjectData & objectData )
    {
        auto & sectorInfo = m_data.sectors[objectData.location.sectorId];
        bool isNew = sectorInfo.objects.insert_or_assign( object.objectId, objectData ).second;
    }


    inline void FromZone::removeObject(
        const SectorRef & sector,
        const ObjectRef & object )
    {
        auto & sectorInfo = m_data.sectors[sector.sectorId];
        sectorInfo.objects.erase( object.objectId );
    }
}
