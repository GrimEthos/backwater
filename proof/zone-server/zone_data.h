#pragma once

#include <cpp/detail/cpp-primitive.h>
#include <cpp/Surface.h>



struct SectorRef
{
    uint32_t                                sectorId;
};


struct ViewRef
{
    uint32_t                                viewId;
};


struct ZoneRef
{
    uint32_t                                sectorId;
    cpp::XY<int>                            zoneId;
};


struct ObjectRef
{
    uint32_t                                objectId;
};


struct NodeRef
{
    ObjectRef                               object;
    uint16_t                                nodeIndex;
};


struct ModuleRef
{
    ObjectRef                               object;
    uint16_t                                moduleIndex;
};


struct ObjectOrientation
{
    cpp::XY<float>                          velocity;
    float                                   angle;
    float                                   spin;
};


struct ObjectBody
{
    uint16_t                                modelType;
    uint16_t                                flags;
};


struct ObjectEffect
{
    float                                   size;
    float                                   growth;
    uint32_t                                timeStart;
    uint32_t                                timeEnd;
};


struct ObjectNode
{
    uint16_t                                parentNodeIndex;
    uint16_t                                type;
    uint16_t                                flags;
    float                                   theta;
    float                                   radius;
};


struct ObjectModule
{
    uint16_t                                nodeIndex;
    uint16_t                                subNodeIndex;
    uint16_t                                moduleType;
    uint16_t                                flags;
};


struct ObjectNodeLink
{
    uint16_t                                node1;
    uint16_t                                node2;
    uint16_t                                linkType;
    uint16_t                                flags;
};


struct ObjectModuleLink
{
    ModuleRef                               module1;    // attacher
    ModuleRef                               module2;    // foundation
    uint16_t                                linkType;
    uint16_t                                flags;
};


struct ObjectData
{
    ObjectBody                              body;
    cpp::XY<double>                         pos;
    ObjectOrientation                       orietation;
    ObjectEffect                            effect;
    std::vector<ObjectNode>                 nodes;
    std::vector<ObjectNodeLink>             nodeLinks;
    std::vector<ObjectModule>               modules;
    std::vector<ObjectModuleLink>           moduleLinks;
};


struct SectorData
{
    uint32_t                                seed;
};


struct ViewData
{
};


struct ZoneData
{
    float                                   speed;
};


enum class Result { Ok, Error };


struct AddObjectReply
{

};


