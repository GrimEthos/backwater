# Proxy Proto

## Component Types

* SessionServer
* ProxyServer
* ServiceServer
* Client

## Interactions

* SessionServer
    * is a singleton and accepts ip connections from ProxyServers
* ProxyServer
    * ip connect to SessionServer
    * recv's ip & session of each ProxyServer
    * ip connect to each ProxyServer
* ProxyClient
    * ip connect to random ProxyServer
    * auth, on success create session with SessionServer
        * SessionServer stores session<->proxy mapping

