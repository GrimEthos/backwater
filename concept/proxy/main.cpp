#include <string>

import proxy;

int main( int argc, const char ** argv )
{
	using namespace grim;

	try {
		SessionServer sessionServer =
			{ "port='3132'" };
		ProxyServer proxyServer[] =
			{
				{ "port='3133' sessionServer='localhost:3132'" },
				{ "port='3134' sessionServer='localhost:3132'" }
			};
		SampleServer sampleServer =
			{ "port='3135' proxyServer='localhost:3133,localhost:3134' svc_name='sample' svc_node='0'" };
		SampleClient client =
			{ "proxyServer='localhost:3133,localhost:3134'" };

		sampleServer.waitForAuth( );
		sampleClient.waitForConnect( );
		
		std::string email = "monkeysmarts@gmail.com";
		sampleClient.auth( email );
		uint64_t serverAddr = sampleClient.lookup( "sample", 0 );
		auto reply = sampleClient.asReply<SampleServer::LookupReply>( sampleClient.request( serverAddr, "" ) );

	} catch ( std::exception & e ) {

	}
	
}