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


int main()
{
	// addModuleType(landing);
	// addModuleType(airlock);
	// addModuleType(corridor);
	// addModuleType(control_room);
	// addModuleLinkType(door)
	// addModuleLinkType(autodoor)
	// addModuleLinkType(airdoor)
	// addModuleLinkType(airvent)
	// addContainerType(peep)
	// addContainerType(module)
	// 
	// addModule(0,landing)
	// addModule(1,landing)
	// addModule(2,airlock)
	// addModule(3,airlock)
	// addModule(4,corridor)
	// addModule(5,control_room)
	// addModuleLink(airdoor, 0, 2)
	// addModuleLink(airdoor, 1, 3)
	// addModuleLink(airdoor, 2, 4)
	// addModuleLink(airdoor, 3, 4)
	// addModuleLink(airdoor, 4, 5)

	return 0;
} 