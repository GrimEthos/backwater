#include <iostream>

#include <cpp/Program.h>
#include <cpp/Log.h>
#include <async/AsyncIO.h>

#include "sector_server.h"
#include "view_server.h"
#include "zone_server.h"



class Prototype
{
public:
    Prototype( );

private:
    void doOutput( );
    void queueOutput( );
    void doFrame( );
    void queueFrame( );

private:
    cpp::AsyncIO io;
    cpp::AsyncTimer outputTimer;
    cpp::AsyncTimer frameTimer;

    std::vector<ViewServer> viewServers;
    std::vector<SectorServer> sectorServers{ 2 };
    std::vector<ZoneServer> zoneServers{ 2 };

    cpp::Time nextFrame;
    int frameCount;
};



int main(int argc, char** argv)
{
    cpp::Program program;
    cpp::Log::addConsoleHandler();
    cpp::Log::addDebuggerHandler();

    try
    { 
        Prototype prototype;

        return 0;
    }
    catch (std::exception& e)
    {
        cpp::Log::error("error: %s\n", e.what());
        return -1;
    }
}


Prototype::Prototype( )
{
    queueOutput( );
    queueFrame( );
    
    io.run( );
}


void Prototype::queueOutput( )
{
    outputTimer = io.waitFor( std::chrono::seconds{ 1 }, std::bind( &Prototype::doOutput, this ) );
}


void Prototype::doOutput( )
{
    queueOutput( );

    cpp::Log::info( "%d frames\n", frameCount );
    frameCount = 0;
}


void Prototype::queueFrame( )
{
    frameTimer = io.waitUntil( nextFrame, std::bind( &Prototype::doFrame, this ) );
}


void Prototype::doFrame( )
{
    frameCount++;

    nextFrame += std::chrono::seconds{ 1 };
    for ( auto & server : zoneServers )
        { nextFrame = std::min( nextFrame, server.runFrame( )); }

    queueFrame( );
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