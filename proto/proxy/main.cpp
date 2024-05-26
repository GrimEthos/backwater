#include <string>
#include <Windows.h>

import cpp.program;
import cpp.windows;
import cpp.asio;
import proxy;
import grim.proto.auth_client;
import grim.proto.session_server;

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

        grim::net::Client client;
        // connecting... done.
        // authenticating.... done.
        // ready
        client.onConnecting( []( std::string_view address ) { cpp::Log( "connecting... " ); } );
        client.onConnect( []( std::string_view address, std::error_code ec, std::string_view reason ) { } );
        client.onAuthing( []( std::string_view email, std::string_view access ) { cpp::Log( "authing... " ); } );
        client.onAuth( []( bool isSuccess, std::string_view failReason ) { } );
        client.onReady( []( uint64_t sessionId )
            {
                    sampleAPI.hello( []( ) { } );
            } );
        client.open( io, address, "monkeysmarts@gmail.com", "[user access]" );

        std::error_code err;
        std::string reason;
        std::chrono::seconds timeout{ 5 };

        std::string address;
        if ( !client.connect( timeout, &address, &err, &reason ) )
        { }

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


        client.close( );
    }
    catch ( std::exception & e ) {
        cpp::Log::error( e.what( ) );
    }

}
