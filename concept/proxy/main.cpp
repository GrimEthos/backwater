#include <string>
#include <Windows.h>

import cpp.program;
import cpp.windows;
import cpp.asio;
import proxy;
import grim.proxy.session;

using namespace grim;

int main( int argc, const char ** argv )
{
    cpp::Program program;
    cpp::Log::addConsoleHandler( );
    cpp::Log::addDebuggerHandler( );

    try {
        cpp::AsyncContext io;

        grim::test::testSessionServerData( );

        /*
        SessionServer sessionServer;
        ProxyServer proxyServer[] =
        {
            { "port='3133' sessionServer='localhost:3132'" },
            { "port='3134' sessionServer='localhost:3132'" }
        };
        SampleServer sampleServer =
            { "proxyServer='localhost:3133,localhost:3134' svc.name = 'sample' svc.node = '0'" };
        SampleClient client =
            { "proxyServer='localhost:3133,localhost:3134'" };

        sessionServer.open( io, "port='3132'" );

        cpp::windows::Kernel32::setConsoleCtrlHandler( [&]( DWORD dwCtrlType )
        {
            sessionServer.close( );
            return true;
        } );
        */

        /*
        sampleServer.waitForAuth( );
        sampleClient.waitForConnect( );

        std::string email = "monkeysmarts@gmail.com";
        sampleClient.auth( email );
        uint64_t serverAddr = sampleClient.lookup( "sample", 0 );
        auto reply = sampleClient.asReply<SampleServer::LookupReply>( sampleClient.request( serverAddr, "" ) );
        */

        io.run( );
    }
    catch ( std::exception & e ) {
        cpp::Log::error( e.what( ) );
    }

}
