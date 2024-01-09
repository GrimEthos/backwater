#include <iostream>
#include <set>

#include <cpp/Program.h>
#include <cpp/Log.h>
#include <async/UdpClient.h>
#include <async/TcpClient.h>

#include <Windows.h>

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


cpp::Memory addressIpOf( cpp::Memory addr )
{
    auto pos = addr.findLastOf( ":" );
    return addr.substr( 0, pos );
}


cpp::Memory addressPortOf( cpp::Memory addr )
{
    auto pos = addr.findLastOf( ":" );
    return addr.substr( pos + 1 );
}

using MessageHandler = std::function<void( Message, const std::string & )>;

struct PeerDesc
{
    int id;
    std::string name;
    std::string extAddr;
    std::string intAddr;
    std::string p2pAddr;
};


struct PeerInfo
{
    int id;
    cpp::Time frameTime;
    uint32_t lastIndex;
    uint32_t currentIndex;
    std::set<uint32_t> lastFrame;
    std::set<uint32_t> currentFrame;
};


class ControlConnection
{
public:
    ControlConnection(
        cpp::AsyncIO & io,
        std::string addr,
        std::string name,
        std::string p2pAddr,
        std::function<void( const PeerDesc & )> connectHandler,
        std::function<void( int )> disconnectHandler );
private:
    void doConnect( );

    void onConnect( std::error_code connectResult );
    void onDisconnect( std::error_code reason );
    void onRecv( std::string & recvBuffer );
private:
    cpp::TcpClient m_tcp;
    cpp::AsyncTimer m_reconnectTimer;
    cpp::AsyncIO & m_io;
    std::string m_addr;
    std::string m_name;
    std::string m_p2pAddr;
    std::map<Message, MessageHandler> m_handlers;
    std::function<void( PeerDesc )> m_connectHandler;
    std::function<void( int )> m_disconnectHandler;
};


class PeerConnection
{
public:
    PeerConnection( cpp::AsyncIO & io );

    std::string getBindAddress( ) const;

    void add( PeerDesc peer );
    void remove( int peerId );

    PeerDesc * lookup( std::string address );

private:
    void onRecv( cpp::Memory from, cpp::Memory data );
    
    void doSend( );
    void send( cpp::Memory data );

    void updatePeerStatus( PeerInfo & peerInfo, uint32_t msgId );
    void checkPeerStatus( PeerInfo & info );

private:
    cpp::AsyncIO & m_io;
    cpp::UdpClient m_udp;
    std::map<int, PeerDesc> m_peers;
    std::map<int, PeerInfo> m_peerInfo;
    std::map<std::string, int> m_peerLookup;

    cpp::AsyncTimer m_sendTimer;
    uint32_t m_sendNum = 0;
};


int main( int argc, const char ** argv )
{
    cpp::Program program;
    cpp::Log::addConsoleHandler( );
    cpp::Log::addDebuggerHandler( );

    try
    {
        char computerName[64];
        DWORD len = 64;
        GetComputerNameExA( ComputerNameNetBIOS, computerName, &len );

        std::string name = ( argc >= 2 ) ? argv[1] : computerName;
        std::string addr = ( argc >= 3 ) ? argv[2] : "home.grimethos.com:7654";

        cpp::AsyncIO io;

        auto peerConnection = PeerConnection{ io };
        auto p2pAddr = peerConnection.getBindAddress( );
        cpp::Log::info( std::format( "udp: {}", p2pAddr ) );

        auto controlConnection = ControlConnection{ io, addr, name, p2pAddr,
            [&]( const PeerDesc & peerDesc ) 
                { 
                    peerConnection.add( peerDesc );
                },
            [&]( int peerId ) 
                { 
                    peerConnection.remove( peerId );
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
    std::string p2pAddr,
    std::function<void( const PeerDesc & )> onConnect,
    std::function<void( int )> onDisconnect ) :
    m_io( io ),
    m_addr( addr ),
    m_name( name ),
    m_p2pAddr( p2pAddr ),
    m_connectHandler( std::move( onConnect ) ),
    m_disconnectHandler( std::move( onDisconnect ) )
{
    m_handlers[Message::Connect] = [=]( Message type, const std::string & data )
    {
        auto parts = cpp::Memory{ data }.split( " " );
        if ( parts.size( ) == 6 )
        {
            PeerDesc peerInfo;
            peerInfo.id = parts[1].asDecimal( );
            peerInfo.name = parts[2];
            peerInfo.intAddr = parts[3];
            peerInfo.extAddr = parts[4];
            peerInfo.p2pAddr = parts[5];
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
        m_tcp.send( std::format( "add {} {} {}\n\n", m_name, m_tcp.localAddress( ), m_p2pAddr ) );
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
        //cpp::Log::info( message );
        if ( auto itr = m_handlers.find( messageType ); itr != m_handlers.end( ) )
            { itr->second( messageType, message ); }
    }
}


PeerConnection::PeerConnection( cpp::AsyncIO & io )
    : m_io(io )
{
    using namespace std::placeholders;
    m_udp.bind( m_io, "", 
        std::bind(&PeerConnection::onRecv, this, _1, _2),
        []( std::error_code reason ) 
        { 
        } );

    doSend( );
}


std::string PeerConnection::getBindAddress( ) const
{
    return m_udp.localAddress();
}


void PeerConnection::add( PeerDesc peer )
{
    m_peers[peer.id] = peer;
    m_peerLookup[peer.p2pAddr] = peer.id;
    
    auto & info = m_peerInfo[peer.id];
    info.id = peer.id;

    cpp::Log::info( std::format( "connect to {} at {}\n",
        peer.name,
        peer.p2pAddr ) );
}


void PeerConnection::remove( int peerId )
{
    if ( auto itr = m_peers.find( peerId ); itr != m_peers.end( ) )
    {
        cpp::Log::info( std::format( "disconnect from {}\n",
            itr->second.name ) );

        m_peerInfo.erase( peerId );
        m_peerLookup.erase( itr->second.p2pAddr );
        m_peers.erase( peerId );
    }
}


PeerDesc * PeerConnection::lookup( std::string address )
{
    auto itr = m_peerLookup.find( address );
    if ( itr == m_peerLookup.end( ) )
        { return nullptr; }
    return &m_peers[itr->second];
}


void PeerConnection::doSend( )
{
    std::string msg;
    msg += cpp::Memory::ofValue( m_sendNum++ );
    msg += cpp::Memory::ofValue( uint32_t{1} );

    send( msg );
    m_sendTimer = m_io.waitFor( cpp::Duration::ofMillis( 20 ), std::bind( &PeerConnection::doSend, this ) );
}


void PeerConnection::send( cpp::Memory data )
{
    for ( auto & peer : m_peers )
    {
        auto & addr = peer.second.p2pAddr;
        m_udp.send( addr, data );
    }
}


void PeerConnection::updatePeerStatus( PeerInfo & info, uint32_t msgIndex )
{
    if ( msgIndex < info.currentIndex )
    {
        info.currentFrame.erase( msgIndex );
        info.lastFrame.erase( msgIndex );
    }
    else
    {
        for ( int i = info.currentIndex; i < msgIndex; i++ )
            { info.currentFrame.insert( i ); }
        info.currentIndex = msgIndex + 1;
    }

    checkPeerStatus( info );
}


void PeerConnection::checkPeerStatus( PeerInfo & info )
{
    auto now = cpp::Time::now( );
    if ( now > info.frameTime )
    {
        auto & addr = m_peers[info.id].p2pAddr;

        std::string msg;
        msg += cpp::Memory::ofValue( m_sendNum++ );
        msg += cpp::Memory::ofValue( uint32_t{ 0 } );
        msg += cpp::Memory::ofValue( info.currentIndex - info.lastIndex );
        msg += cpp::Memory::ofValue( uint32_t{ (uint32_t)info.lastFrame.size( ) } );

        m_udp.send( addr, msg );

        info.lastFrame = std::move( info.currentFrame );
        info.lastIndex = info.currentIndex;
        info.frameTime += std::chrono::milliseconds( 1000 );
    }
}


void PeerConnection::onRecv( cpp::Memory from, cpp::Memory data )
{
    PeerDesc * peerDesc = lookup( from );
    if ( peerDesc )
    {
        auto & info = m_peerInfo[peerDesc->id];

        uint32_t * msgData = (uint32_t *)data.begin( );

        uint32_t msgIndex = msgData[0];
        updatePeerStatus( info, msgIndex );

        uint32_t msgType = msgData[1];
        if ( msgType == 0 )
        {
            uint32_t total = msgData[2];
            uint32_t lost = msgData[3];
            double dropped = (total > 0) ? lost / total : 100.0;

            //if ( dropped > 0.0 )
            {
                cpp::Log::info( std::format( "from {} : {}/{}%\n",
                    peerDesc->name,
                    total,
                    dropped ) );
            }
        }
        else if ( msgType == 1 )
        {

        }
    }
}
