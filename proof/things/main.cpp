#include <vector>
#include <map>
#include "thing.h"



struct Game
{
	std::map<Things::TypeId, Things>				thingTypes;
	std::map<Containers::TypeId, Containers>		containerTypes;
	std::map<Modules::TypeId, Modules>				moduleTypes;
	std::map<ModuleLinks::TypeId, ModuleLinks>		moduleLinkTypes;
	
	std::map<std::string, ThingFlags>				thingFlags;
	std::map<std::string, ContainerFlags>			containerFlags;
	std::map<std::string, ModuleFlags>				moduleFlags;
	std::map<std::string, ModuleLinkFlags>			moduleLinkFlags;
	
	std::map<Container::Id, Container>				containers;
	std::map<Container::Id, std::vector<Thing>>		things;
	std::map<Module::Id, Module>					modules;
	std::map<Module::Id, std::vector<ModuleLink>>	moduleLinks;
	std::map<Peep::Id, Peep>						peeps;
};


void main()
{
	// module
	//
} 