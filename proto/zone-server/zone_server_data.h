#pragma once

#include <map>
#include <set>
#include "zone_interfaces.h"
#include "sim.h"


namespace zone_server
{
    class Sim;
    struct Data
    {
        struct ZoneMeta
        {
            IZoneToZoneServer::ptr_t            izone;
            ZoneData                            zoneData;
            Sim                                 sim;
        };
        struct SectorMeta
        {
            IZoneToSectorServer::ptr_t          isector;
            SectorData                          sectorData;
            std::map<cpp::XY<int>, ZoneMeta>    zones;
            std::map<uint32_t, ObjectData>      objects;
        };
        struct ViewMeta
        {
            IZoneToViewServer::ptr_t            iview;
            ViewData                            viewData;
        };
        std::map<uint32_t, SectorMeta>          sectors;
        std::map<uint32_t, ViewMeta>            views;
        cpp::Time                               time;
        int                                     fps = 128;

        const SectorMeta *                      getSectorMeta( uint32_t sectorId ) const;
        bool                                    hasZone( ZoneRef zone, const SectorMeta * sector = nullptr ) const;
        bool                                    hasObject( ObjectRef object ) const;
    };


    ////////////////////////////////////////////////////////////

    const Data::SectorMeta * Data::getSectorMeta( uint32_t sectorId ) const
    {
        auto itr = sectors.find( sectorId );
        return ( itr != sectors.end( ) )
            ? &( itr->second )
            : nullptr;
    }


    bool Data::hasZone( ZoneRef zone, const SectorMeta * sectorMeta ) const
    {
        if ( !sectorMeta )
            { sectorMeta = getSectorMeta( zone.sectorId ); }
        if ( !sectorMeta )
            { return false; }

        auto & zones = sectorMeta->zones;
        auto zoneItr = zones.find( zone.zoneId );
        if ( zoneItr != zones.end( ) )
            { return zoneItr->second.izone == nullptr; }

        return false;
    }


    bool Data::hasObject( ObjectRef object ) const
    {
        return true;
    }




}