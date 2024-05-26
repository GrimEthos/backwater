module;

#include <cstdint>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <functional>

export module grim.proto.view;

import cpp.memory;

// component, module,

export namespace grim
{
    struct Object
    {
        using                               Id = uint32_t;
        using                               Array = std::vector<Object>;
        using                               Set = std::set<Id>;
        using                               Map = std::map<Id, Object>;
        Id                                  id;
    };

    struct Client
    {
        struct View
        {
            Object::Id                      origin;
            Object::Id                      focus;
            Object::Map                     objects;
        };
    };

    struct Console
    {

    };

    //! The View is responsible for:
    //! * what the client can see
    //! * the relative object ids
    //! * the relative spacial information
    //! * converting control info to acceleration & passing to the Zone (like turning, thrust, breaking, pulling, and pushing)
    struct View
    {
        using                               Id = uint32_t;
        using                               IdMap = std::map<Object::Id, Object::Id>;
        Object::Id                          origin;         // object that provides spacial reference
        float                               originTheta;
        IdMap                               idMap;          // objects in view and id for client
    };

    struct Sector
    {

    };


    //! multiple Views may tract the same object
    //!     - 
    struct Zone
    {
        struct ViewObject
        {
            enum class                      Type { Normal, All };
            Object::Id                      id;         // object id that is reporting for view
            Type                            type;       // type of reporting (e.g. should include obstruction, interference, other calculations)
            float                           range;      // objects view range
        };
        using                               ViewObjects = std::map<View::Id, Object::Map>;
        using                               ViewObjects = std::map<View::Id, Object::Map>;
        ViewObjects                         viewObjects;  // focus objects?
        Object::Map                         objects;
    };
}