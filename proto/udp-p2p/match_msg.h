#pragma once

#include <string>
#include <functional>



enum class Message
    { Unknown, Add, Connect, Disconnect };


bool getMessage( std::string & buffer, Message & messageType, std::string & message )
{
    size_t pos = buffer.find( "\n\n" );
    if ( pos == std::string::npos )
        { return false; }

    message = buffer.substr( 0, pos );
    if ( message.starts_with( "add" ) )
        { messageType = Message::Add; }
    else if ( message.starts_with( "connect" ) )
        { messageType = Message::Connect; }
    else if ( message.starts_with( "disconnect" ) )
        { messageType = Message::Disconnect; }
    else
        { messageType = Message::Unknown; }
    buffer = buffer.substr( pos + 2 );

    return true;
}


using MessageHandler = std::function<void( Message, const std::string & )>;
