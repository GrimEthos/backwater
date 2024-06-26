#include <string>
#include <Windows.h>

import cpp.program;
import cpp.windows;
import cpp.asio;
import proxy;
import grim.net;
import grim.proto.auth_client;
import grim.proto.session_server;

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

        grim::net::Client client;
        // connecting... done.
        // authenticating.... done.
        // ready
        client.onConnecting( []( grim::net::StrArg address ) { cpp::Log::info( "connecting... " ); } );
        client.onConnect( []( grim::net::StrArg addr, grim::net::Result result, std::string reason ) { cpp::Log::info( "connect - {}", reason ); } );
        client.onAuthing( []( grim::net::StrArg addr, grim::net::StrArg access ) { cpp::Log::info( "authing... " ); } );
        client.onAuth( []( grim::net::StrArg email, uint64_t sessionId, grim::net::Result result, std::string reason ) { cpp::Log::info( "auth. " ); } );
        client.onReady( []( grim::net::StrArg addr, grim::net::Result result, std::string reason )
            {
                cpp::Log::info( "ready. " );
                //sampleAPI.hello( []( ) { } );
            } );
        client.onDisconnect( []( grim::net::StrArg addr, grim::net::Result result, std::string reason ) { cpp::Log::info( "disconnect - {}", reason ); } );
        client.open( io, "localhost:65432", "monkeysmarts@gmail.com", "[user access]" );

        grim::net::Result result;
        std::string reason;

        std::string address;
        if ( !client.connect( 5, &address, &result, &reason ) )
            { client.close( ); }

        /*
        std::string email;
        uint64_t sessionId;
        if ( !client.auth( timeout, &email, &sessionId, &err, &reason ) )
        { }

        //! maybe wait for client ready notification
        if ( !client.ready( timeout, &err, &reason ) )
        { }

        grim::GalaxyAPI galaxy;
        galaxy.attach( client );
        galaxy.hello( );

        grim::ShareAPI share;
        share.attach( client );
        share.onFriend( []( ) { } );
        share.onFileUpdate( []( ) { } );

        share.hello( );
        share.updateFriend( "monkeysmarts@gmail.com" );
        share.peek( "monkeysmarts@gmail.com" );
        share.addFollow( "monkeysmarts@gmail.com", "wall" );
        */
        cpp::windows::Kernel32::setConsoleCtrlHandler( [&]( DWORD dwCtrlType )
            {
                io.post( [&]( ) { client.close( ); } );
                return true;
            } );
        io.run( );

        client.close( );
    }
    catch ( std::exception & e ) {
        cpp::Log::error( e.what( ) );
    }

}
