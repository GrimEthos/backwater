#include <cinttypes>
#include <filesystem>

import cpp.program;
import cpp.asio;
import grim.auth;
import grim.net;

int main(int argc, char ** argv)
{
    cpp::Program program;
    cpp::AsyncContext io;

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
    if (!client.connect( timeout, &address, &err, &reason ))
        { }

    std::string email;
    uint64_t sessionId;
    if (!client.auth( timeout, &email, &sessionId, &err, &reason ))
        { }

    //! maybe wait for client ready notification
    if (!client.ready( timeout, &err, &reason ))
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