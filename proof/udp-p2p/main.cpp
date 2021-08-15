#include <iostream>

#include <cpp/Program.h>
#include <cpp/Log.h>
#include <async/UdpClient.h>
#include <async/TcpClient.h>


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

struct PeerInfo
{
    int id;
    std::string name;
    std::string extAddr;
    std::string intAddr;
};


class ControlConnection
{
public:
    ControlConnection(
        cpp::AsyncIO & io,
        std::string addr,
        std::string name,
        std::function<void( const PeerInfo & )> connectHandler,
        std::function<void( int )> disconnectHandler );
private:
    void doConnect( );
    void doRecv( );

    void onConnect( std::error_code connectResult );
    void onDisconnect( std::error_code reason );
    void onRecv( std::string & recvBuffer );
private:
    cpp::TcpClient m_tcp;
    cpp::AsyncTimer m_reconnectTimer;
    cpp::AsyncIO & m_io;
    std::string m_addr;
    std::string m_name;
    std::map<Message, MessageHandler> m_handlers;
    std::function<void( PeerInfo )> m_connectHandler;
    std::function<void( int )> m_disconnectHandler;
};


class PeerConnection
{
public:
private:
    cpp::UdpClient m_udp;
};


int main( int argc, const char ** argv )
{
    cpp::Program program;
    cpp::Log::addConsoleHandler( );
    cpp::Log::addDebuggerHandler( );

    try
    {
        std::string name = ( argc >= 2 ) ? argv[1] : "tom";
        std::string addr = ( argc >= 3 ) ? argv[2] : "127.0.0.1:7654";

        cpp::AsyncIO io;

        auto controlConnection = ControlConnection{ io, addr, name,
            []( const PeerInfo & peerInfo ) 
                { 
                },
            []( int peerId ) 
                { 
                } };

        io.run( );
        return 0;
    }
    catch ( std::exception & e )
    {
        cpp::Log::error( "error: %s\n", e.what( ) );
        return -1;
    }

}


ControlConnection::ControlConnection(
    cpp::AsyncIO & io,
    std::string addr,
    std::string name,
    std::function<void( const PeerInfo & )> onConnect,
    std::function<void( int )> onDisconnect ) :
    m_io( io ),
    m_addr( addr ),
    m_name( name ),
    m_connectHandler( std::move( onConnect ) ),
    m_disconnectHandler( std::move( onDisconnect ) )
{
    m_handlers[Message::Connect] = [=]( Message type, const std::string & data )
    {
        auto parts = cpp::Memory{ data }.split( " " );
        if ( parts.size( ) == 5 )
        {
            PeerInfo peerInfo;
            peerInfo.id = parts[1].asDecimal( );
            peerInfo.name = parts[2];
            peerInfo.intAddr = parts[3];
            peerInfo.extAddr = parts[4];
            m_connectHandler( peerInfo );
        }
    };
    m_handlers[Message::Disconnect] = [=]( Message type, const std::string & data )
    {
        auto parts = cpp::Memory{ data }.split( " " );
        if ( parts.size( ) == 2 )
        {
            int peerId = parts[1].asDecimal( );
            m_disconnectHandler( peerId );
        }
    };
    doConnect( );
}


void ControlConnection::doConnect( )
{
    using namespace std::placeholders;

    m_tcp.connect( m_io, m_addr,
        std::bind( &ControlConnection::onConnect, this, _1 ),
        std::bind( &ControlConnection::onRecv, this, _1 ),
        std::bind( &ControlConnection::onDisconnect, this, _1 ) );
}


void ControlConnection::onConnect( std::error_code connectResult )
{
    cpp::Log::info( std::format( "onConnect() : {}\n",
        connectResult.message( ) ) );
    if ( !connectResult )
    {
        m_tcp.send( std::format( "add {} {}\n\n", m_name, m_tcp.localAddress( ) ) );
    }
    else
    {
        m_reconnectTimer = m_io.waitFor( cpp::Duration::ofSeconds( 1 ), std::bind( &ControlConnection::doConnect, this ) );
    }
}


void ControlConnection::onDisconnect( std::error_code reason )
{
    cpp::Log::info( std::format( "onDisconnect() : {}\n",
        reason.message( ) ) );
}


void ControlConnection::onRecv( std::string & recvBuffer )
{
    Message messageType;
    std::string message;
    while ( getMessage( recvBuffer, messageType, message ) )
    {
        cpp::Log::info( message );
        if ( auto itr = m_handlers.find( messageType ); itr != m_handlers.end( ) )
        { itr->second( messageType, message ); }
    }
}

