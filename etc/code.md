# code

## goals
* arch - attempt to describe architecture using code
* lib - encapsulate arch, client, and server code as static libraries
* test - all lib code is testable using a single process, including integration testing

## namespaces
* grim::net - client, proxy_server, session_serer
* grim::auth - client, server, emailer
* grim::game - client, servers

## components

* grimnet
	* client
	* proxy_server
	* session_server
* grimauth
	* client
	* server
* backwater
	* arch
		* sim
		* things
	* client
		* console
		* sim_view
		* zone_server
	* server
		* galaxy_server
		* content_server - user, story, scenario
		* sector_server
		* view_server
		* console_server
