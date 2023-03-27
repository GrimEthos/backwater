module;

#include <string>

export module proxy;

export namespace grim
{
	class SessionServer
	{
	public:
		SessionServer( std::string config );
	};

	class ProxyServer
	{
	public:
		ProxyServer( std::string config);
	};

	class SampleServer
	{
	public:
		SampleServer( std::string config );

		struct LookupReply
		{

		};
	};
}
