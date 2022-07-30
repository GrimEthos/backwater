Peep::Id								peep;
#pragma once

#include <cinttypes>
#include <string>



//! Things = set of thing types
//!		e.g. hand_gun, flowers, ring, shield
//! things[type] { name='' parentType='' name='' flags='' }
struct Things
{
	using TypeId = uint16_t;
	using Flags = uint64_t;
	TypeId									type;
	TypeId									parentType;
	std::string								name;
	Flags									flags;
};


//! Containers = set of container types
//!		e.g. inventory, crate, barrel, wearable, equipable
//! containers[type] { name='' flags='' }
struct Containers
{
	using TypeId = uint16_t;
	using Flags = uint64_t;
	TypeId									type;
	std::string								name;
	Flags									flags;
};


//! modules[type] { name='' flags='' }
struct Modules
{
	using TypeId = uint16_t;
	using Flags = uint64_t;
	TypeId									type;
	std::string								name;
	Flags									flags;
};


//! module_links[type] { name='' flags='' }
struct ModuleLinks
{
	using TypeId = uint16_t;
	using Flags = uint64_t;
	TypeId									type;
	std::string								name;
	Flags									flags;
};


//! thing_flags[name] { mask='' }
struct ThingFlags
{
	std::string								name;
	uint64_t								mask;
};


//! container_flags[name] { mask='' }
struct ContainerFlags
{
	std::string								name;
	uint64_t								mask;
};


//! module_flags[name] { mask='' }
struct ModuleFlags
{
	std::string								name;
	uint64_t								mask;
};


//! module_link_flags[name] { mask='' }
struct ModuleLinkFlags
{
	std::string								name;
	uint64_t								mask;
};


//! Container = special thing that contains other containers or things
//! container[id] { type='' thingType='' parent='' capacity='' }
struct Container
{
	using Id = uint32_t;
	Id										id;
	Containers::TypeId						containerType;
	Things::TypeId							thingType;
	Id										parentId; // contained in
	uint32_t								capacity;
};


//! Thing = anything that goes into a container
//! thing[containerId][index] { type='' count='' }
struct Thing
{
	Container::Id							containerId;	// what is it contained in
	Things::TypeId							thingType;
	uint32_t								count;
};


//! module[id] { type='' thingType='' containerId='' }
struct Module
{
	using Id = uint32_t;
	Module::Id								id;
	Modules::TypeId							type;
	Things::TypeId							thingType;
	Container::Id							containerId;	// contained things
};


//! module_link[id][linkId] { type='' thingType='' }
struct ModuleLink
{
	Module::Id								id;
	Module::Id								linkId;
	ModuleLinks::TypeId						type;
	Things::TypeId							thingType;
};


//! peep[id] { location='' containerId='' }
struct Peep
{
	using Id = uint32_t;
	Id										id;
	Module::Id								location;
	Container::Id							containerId;	// inventory
};


//! know_module[peep][thing] { flags='0' }
struct KnowThings
{
	Peep::Id								peep;
	Things::TypeId							thing;
	uint64_t								flags;
};


//! know_module[peep][id] { flags='0' }
struct KnowModule
{
	Peep::Id								peep;
	Module::Id								id;
	uint64_t								flags;
};