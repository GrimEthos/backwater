#include <map>
#include <functional>
#include <memory>

#include <cpp/Memory.h>
#include <cpp/Program.h>
#include <async/TcpServer.h>

#include <cpp/detail/cpp-win32.h>
#define ASIO_STANDALONE
#include <async/lib/asio-1.18.0/include/asio.hpp>

struct Node
{
    int id;
    std::string name;
    std::string intAddr;
    std::string extAddr;
    std::string p2pAddr;
    bool isLocal;
};


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


class MatchMaker
{
public:
    MatchMaker( cpp::AsyncIO & io, int port, std::string lanSubnet, std::string wanAddress );

private:
    void onConnect( std::error_code acceptError, const std::string & addr );
    void onRecv( const std::string & addr, std::string & recvBuffer );
    void onDisconnect( const std::string & addr, std::error_code reason );

    void add( const std::string & addr, cpp::Memory msg );
    void remove( const std::string & addr );

    static cpp::Memory ipOf( cpp::Memory addr );
    static cpp::Memory portOf( cpp::Memory addr );

private:
    cpp::TcpServer m_server;
    std::string m_wanIp;
    std::string m_lanSubnet;
    int m_nextId = 1;
    std::map<int, Node> m_nodes;
    std::map<std::string, int> m_addrNodes;
};



int main( int argc, const char ** argv )
{
    cpp::Program program{ argc, argv };

    try
    {
        cpp::Log::addConsoleHandler( );
        cpp::Log::addDebuggerHandler( );

        const char * wanAddress = "home.grimethos.com";
        const char * lanSubnet = "192.168.0.0/16";

        cpp::AsyncIO io;
        auto matchMaker = MatchMaker{ io, 7654, lanSubnet, wanAddress };
        io.run( );
    }
    catch ( std::exception & e )
    {
        cpp::Log::error( e.what( ) );
    }

    return 0;
}


MatchMaker::MatchMaker( cpp::AsyncIO & io, int port, std::string subnet, std::string wanAddress )
{
    m_wanIp = cpp::TcpServer::resolve( cpp::TcpVersion::v4, wanAddress );
    m_lanSubnet = subnet;

    using namespace std::placeholders;
    m_server.open( io, 7654, 
        std::bind( &MatchMaker::onConnect, this, _1, _2 ),
        std::bind( &MatchMaker::onRecv, this, _1, _2 ),
        std::bind( &MatchMaker::onDisconnect, this, _1, _2 ), 
        "0.0.0.0", cpp::TcpVersion::v4 );
}


void MatchMaker::onConnect( std::error_code acceptError, const std::string & addr )
{
    //cpp::Log::info( std::format( "onConnect({} - {}\n", addr, acceptError.message( ) ) );
}


void MatchMaker::onRecv( const std::string & addr, std::string & recvBuffer )
{
    size_t pos;
    while ( ( pos = recvBuffer.find( "\n\n" ) ) != std::string::npos )
    {
        auto msg = cpp::Memory{ recvBuffer }.substr( 0, pos );
        if ( msg.beginsWith( "add" ) )
        {
            add( addr, msg );
        }
        else
        {
            m_server.send( addr, "invalid request\n\n" );
            m_server.disconnect( addr, std::make_error_code( std::errc::bad_message ) );
        }

        recvBuffer = recvBuffer.substr( pos + 2 );
    }
}


void MatchMaker::onDisconnect( const std::string & addr, std::error_code reason )
{
    remove( addr );

    //cpp::Log::info( std::format( "onDisconnect({}) - {}\n", addr, reason.message( ) ) );
}


void MatchMaker::add( const std::string & addr, cpp::Memory msg )
{
    auto parts = msg.split( " " );
    if ( parts.size( ) != 4 )
    { 
        m_server.send( addr, "add : invalid args\n\n" );
        m_server.disconnect( addr, std::make_error_code( std::errc::bad_message ) );
        return;
    }

    int nodeId = m_nextId++;

    m_addrNodes[addr] = nodeId;

    auto & node = m_nodes[nodeId];
    node.id = nodeId;
    node.name = parts[1];
    node.extAddr = addr;
    node.intAddr = parts[2];
    node.p2pAddr = parts[3];

    auto localNetwork = asio::ip::make_network_v4( m_lanSubnet );
    node.isLocal = ( localNetwork.hosts( ).find( asio::ip::make_address_v4( addr ) ) != localNetwork.hosts( ).end( ) );

    m_server.send( addr, "add : ok\n\n" );

    for ( auto itr : m_nodes )
    {
        auto & otherNode = itr.second;
        if ( otherNode.id == nodeId )
            { continue; }

        bool isSameLan = ( node.isLocal && otherNode.isLocal ) || addressIpOf( node.extAddr ) == addressIpOf( otherNode.extAddr );

        std::string p2pAddr = ( otherNode.isLocal || isSameLan )
            ? addressIpOf( otherNode.intAddr ) 
            : addressIpOf( otherNode.extAddr );

        if ( otherNode.isLocal && !isSameLan )
            { p2pAddr = m_wanIp; }

        m_server.send( addr, std::format( "connect {} {} {} {} {}\n\n",
            otherNode.id,
            otherNode.name,
            otherNode.intAddr,
            otherNode.extAddr,
            p2pAddr + ":" + addressPortOf( otherNode.p2pAddr ) ) );

        p2pAddr = ( node.isLocal || isSameLan )
            ? addressIpOf( node.intAddr )
            : addressIpOf( node.extAddr );

        if ( node.isLocal && !isSameLan )
            { p2pAddr = m_wanIp; }

        m_server.send( otherNode.extAddr, std::format( "connect {} {} {} {} {}\n\n",
            node.id,
            node.name,
            node.intAddr,
            node.extAddr,
            p2pAddr + ":" + addressPortOf( node.p2pAddr ) ) );
    }

    cpp::Log::info( std::format( "add node: {} {} {} {} {}\n",
        node.id,
        node.name,
        node.extAddr,
        node.intAddr,
        node.p2pAddr ) );
}


void MatchMaker::remove( const std::string & addr )
{
    int nodeId = m_addrNodes[addr];
    for ( auto itr : m_nodes )
    {
        auto & node = itr.second;
        if ( node.id == nodeId )
            { continue; }

        m_server.send( node.extAddr, std::format( "disconnect {}\n\n", nodeId ) );
    }

    auto & node = m_nodes[nodeId];
    cpp::Log::info( std::format( "remove node: {} {} {} {} {}\n",
        node.id,
        node.name,
        node.extAddr,
        node.intAddr,
        node.p2pAddr ) );

    m_nodes.erase( nodeId );
    m_addrNodes.erase( addr );
}


