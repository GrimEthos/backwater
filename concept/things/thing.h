uint32_t								count;
Peep::Id								peep;
#pragma once

#include <cinttypes>
#include <string>



struct Vector3
{
	float									x, y, z;
};


struct Symbol
{
	using TypeId = uint16_t;
	TypeId									type;
	TypeId									parentType;
	std::string								name;
};


struct ThingType
{
	using Flags = uint16_t;
	Symbol::TypeId							type;
	Vector3									dim;
	Flags									flags;
};


struct ThingContainerType
{
	using Flags = uint16_t;
	Symbol::TypeId							type;
	Flags									flags;
};


struct ThingContainer
{
	using Id = uint32_t;
	Id										id;
	Symbol::TypeId							type;
	Id										parentId; // contained in
};


struct ContainedThing
{
	ThingContainer::Id						container;
	Symbol::TypeId							type;
	uint32_t								count;
};


struct UniqueThing
{
	using Id = uint32_t;
	Id										id;
	ThingContainer::Id						container;
	Symbol::TypeId							type;
	uint64_t								visual;
};


struct ModuleType
{
	using Flags = uint16_t;
	Symbol::TypeId							type;
	Flags									flags;
};


struct ModuleLinkType
{
	using Flags = uint64_t;
	Symbol::TypeId							type;
	Flags									flags;
};


struct Blueprint
{
	Symbol::TypeId							type;
	Symbol::TypeId							language;
};


struct BlueprintThing
{
	Symbol::TypeId							blueprintType;
	Symbol::TypeId							ingredientType;
	uint32_t								count;
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


//! module[id] { type='' thingType='' containerId='' }
struct Module
{
	using Id = uint32_t;
	Id										id;
	Symbol::TypeId							type;
	ThingContainer::Id						inventory;	// contained things
};


//! module_link[id][linkId] { type='' thingType='' }
struct ModuleLink
{
	Module::Id								id;
	Module::Id								linkId;
	Symbol::TypeId							type;
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


struct 