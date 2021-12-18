#include <iostream>

#include <cpp/Program.h>
#include <cpp/Log.h>
#include <async/AsyncIO.h>

#include "sector_server.h"
#include "view_server.h"
#include "zone_server.h"


int main(int argc, char** argv)
{
    cpp::Program program;
    cpp::Log::addConsoleHandler();
    cpp::Log::addDebuggerHandler();

    try
    { 
        cpp::AsyncIO io;

        SectorServer sectorServer[2];
        ZoneServer zoneServer[2];
        ViewServer viewServer;

        sectorServer[0].fromZone.removeObject( ObjectRef{ 32 } );

        io.post([]() { printf("step 1\n"); });
        io.post([]() { printf("step 2\n"); });
        io.post([]() { printf("step 3\n"); });

        io.run();

        return 0;
    }
    catch (std::exception& e)
    {
        cpp::Log::error("error: %s\n", e.what());
        return -1;
    }
}


//! small ship
//! * nodes
//!     * front mount
//!     * left mount
//!     * right mount
//!     * left engine
//!     * right engine
//!     * n1
//!         * control
//!     * n2   
//!         * crawl space
//!     * n3
//!         * engineering
//!     * crawl space
//!         * 
//!         
//! * modules
//!     * control
//!     * crawl space
//!     * left cargo bay
//!     * right cargo bay
//!     * engineering
//!     * left component bay
//!     * right component bay



//!
//! # Entities
//!     * Sector - has persistent ownership of all objects
//!     * ZoneServer - has temporary ownership of zones and all objects in them
//!     * SubZone - has temporary ownership of objects
//! 
//! # Object Movement
//!     * a subzone will simulate its objects
//!     * a subzone will simulate objects adjacent to its boundaries (boundary-size)
//!     * a subzone will handle collisions that occur in it
//!     * a subzone notifies adjacent subzones of object changes within its boundary area (acceleration & collision only, not pos)
//! 