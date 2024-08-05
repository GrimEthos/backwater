#include <string>
#include <Windows.h>

import cpp.program;
import cpp.windows;
import cpp.asio;
import grim.net;

int main( int argc, const char ** argv )
{
    cpp::Program program;
    cpp::Log::addConsoleHandler( );
    cpp::Log::addDebuggerHandler( );

    try 
    {
        cpp::AsyncContext io;

        grim::net::test::testSessionServerData( );

        grim::net::SessionServer sessionServer;
        sessionServer.open( io, "127.0.0.1:65432", "[::1]:65432", "monkeysmarts@gmail.com" );

        grim::net::Result result;
        std::string email;
        uint64_t sessionId = 0;
        if ( !sessionServer.auth( 5, &email, &sessionId, &result ) )
            { return 1; }
        if (!sessionServer.ready( 5, &result ))
            { return 1; }

        grim::net::ProxyServer proxyServer[2];
        proxyServer[0].open( io, "127.0.0.1:43210", "[::1]:43210", "[::1]:65432", "monkeysmarts@gmail.com", 0 );
        proxyServer[1].open( io, "127.0.0.1:43211", "[::1]:43211", "[::1]:65432", "monkeysmarts@gmail.com", 1 );
        if ( !proxyServer[0].auth( 5, &email, &sessionId, &result ) || !proxyServer[1].auth( 5, &email, &sessionId, &result ) )
            { return 1; }
        if ( !proxyServer[0].ready( 5, &result ) || !proxyServer[1].ready( 5, &result ) )
            { return 1; }

        /*
        ProxyServer proxyServer[] =
        {
            { "port='3133' sessionServer='localhost:3132'" },
            { "port='3134' sessionServer='localhost:3132'" }
        };
        SampleServer sampleServer =
            { "proxyServer='localhost:3133,localhost:3134' svc.name = 'sample' svc.node = '0'" };
        SampleClient client =
            { "proxyServer='localhost:3133,localhost:3134'" };
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
        client.onConnecting( []( grim::net::StrArg address ) 
            { cpp::Log::info( "connecting... " ); } );
        client.onConnect( []( grim::net::StrArg addr, grim::net::Result result, std::string reason ) 
            { cpp::Log::info( "connect - {}", reason ); } );
        client.onAuthing( []( grim::net::StrArg addr, grim::net::StrArg access ) 
            { cpp::Log::info( "authing... " ); } );
        client.onAuth( []( grim::net::StrArg email, uint64_t sessionId, grim::net::Result result ) 
            { cpp::Log::info( "auth: {}", email.view ); } );
        client.onReady( []( grim::net::Result result )
            {
                cpp::Log::info( "ready. " );
                //sampleAPI.hello( []( ) { } );
            } );
        client.onDisconnect( []( grim::net::StrArg addr, grim::net::Result result, std::string reason ) { cpp::Log::info( "disconnect - {}", reason ); } );
        client.open( io, "localhost:65432", "monkeysmarts@gmail.com", "[user access]" );

        std::string reason;
        std::string address;

        if ( !client.connect( 5, &address, &result, &reason ) )
            { client.close( ); }
        if ( !client.auth( 5, &email, &sessionId, &result ) )
            { client.close( ); }
        if ( !client.ready( 5, &result ) )
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
                cpp::Log::info( "ctrl+c" );
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
