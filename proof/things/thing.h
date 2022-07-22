#pragma once

#include <cinttypes>
#include <string>



//! Things = set of thing types
//!		e.g. hand_gun, flowers, ring, shield
struct Things
{
	using TypeId = uint16_t;
	using Flags = uint64_t;
	TypeId									type;
	TypeId									classType;
	std::string								name;
	Flags									flags;
};


//! Containers = set of container types
//!		e.g. chest, backpack, building, closet
struct Containers
{
	using TypeId = uint16_t;
	using Flags = uint64_t;
	TypeId									type;
	std::string								name;
	Flags									flags;
};


struct Modules
{
	using TypeId = uint16_t;
	using Flags = uint64_t;
	TypeId									type;
	std::string								name;
	Flags									flags;
};


struct ModuleLinks
{
	using TypeId = uint16_t;
	using Flags = uint64_t;
	TypeId									type;
	std::string								name;
	Flags									flags;
};


struct ThingFlags
{
	std::string								name;
	uint64_t								mask;
};


struct ContainerFlags
{
	std::string								name;
	uint64_t								mask;
};


struct ModuleFlags
{
	std::string								name;
	uint64_t								mask;
};


struct ModuleLinkFlags
{
	std::string								name;
	uint64_t								mask;
};


//! Container = special thing that contains other containers or things
struct Container
{
	using Id = uint32_t;
	Id										id;
	Id										containerId;
	Things::TypeId							thingType;
	Containers::TypeId						containerType;
	uint32_t								capacity;
};


//! Thing = anything that goes into a container
struct Thing
{
	Container::Id							containerId;
	Things::TypeId							thingType;
	uint32_t								count;
};


struct Module
{
	using Id = uint32_t;
	Module::Id								id;
	Modules::TypeId							type;
	Container::Id							containerId;
};


struct ModuleLink
{
	Module::Id								id;
	Module::Id								linkId;
	ModuleLinks::TypeId						type;
};


struct Peep
{
	using Id = uint32_t;
	Id										id;
	Module::Id								location;
	Container::Id							containerId; // inventory
};