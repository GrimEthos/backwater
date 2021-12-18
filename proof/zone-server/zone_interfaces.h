#pragma once

#include <functional>

#include "zone_data.h"



struct IZoneToSectorServer
{
    using ptr_t = std::unique_ptr<IZoneToSectorServer>;

    virtual void                            addObject(
                                                const ObjectRef & object,
                                                const ZoneRef & zone,
                                                const ObjectData & objectData,
                                                std::function<void( Result, AddObjectReply )> result ) = 0;
    virtual void                            updateObject(
                                                const ObjectRef & object,
                                                const ZoneRef & zone,
                                                const ObjectData & objectData ) = 0;
    virtual void                            removeObject(
                                                const ObjectRef & object ) = 0;
};


struct IZoneToViewServer
{
    using ptr_t = std::unique_ptr<IZoneToViewServer>;

    virtual void                            updateObject(
                                                const ObjectRef & object,
                                                const ZoneRef & zone,
                                                const ObjectData & objectData ) = 0;
    virtual void                            removeObject(
                                                const ObjectRef & object ) = 0;
};


struct IZoneToZoneServer
{
    using ptr_t = std::unique_ptr<IZoneToZoneServer>;

    virtual void                            updateObject(
                                                const ObjectRef & object,
                                                const ZoneRef & zone,
                                                const ObjectData & objectData ) = 0;
    virtual void                            removeObject(
                                                const ObjectRef & object ) = 0;
};


struct ISectorToZoneServer
{
    using ptr_t = std::unique_ptr<ISectorToZoneServer>;

    virtual void                            updateSectorInfo(
                                                const SectorRef & sector,
                                                IZoneToSectorServer::ptr_t isector,
                                                const SectorData & sectorData ) = 0;
    virtual void                            removeSectorInfo(
                                                SectorRef & sector ) = 0;

    virtual void                            updateZoneInfo(
                                                const ZoneRef zone,
                                                IZoneToZoneServer::ptr_t izone,
                                                const ZoneData & zoneData ) = 0;
    virtual void                            removeZoneInfo(
                                                const ZoneRef zone ) = 0;

    virtual void                            updateViewInfo(
                                                const ViewRef & view,
                                                IZoneToViewServer::ptr_t iview,
                                                const ViewData & viewData ) = 0;
    virtual void                            removeViewInfo(
                                                const ViewRef & view ) = 0;

    virtual void                            updateObject(
                                                const ObjectRef & object,
                                                const ZoneRef & zone,
                                                const ObjectData & objectData ) = 0;
    virtual void                            removeObject(
                                                const ObjectRef & object ) = 0;
};


struct ISectorToViewServer
{
    using ptr_t = std::unique_ptr<ISectorToViewServer>;
};


struct IViewToZoneServer
{
    virtual void                            updateWatch(
                                                const ObjectRef & object,
                                                float distance ) = 0;
    virtual void                            removeWatch(
                                                const ObjectRef & object ) = 0;
    virtual void                            controlObject(
                                                const ObjectRef & object,
                                                uint32_t timestamp,
                                                int inputCount,
                                                int inputs[],
                                                float values[] ) = 0;
};


struct IViewToSectorServer
{

};

