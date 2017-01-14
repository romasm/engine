#pragma once

#include "Common.h"

#define ENTITY_INDEX_BITS			16
#define ENTITY_INDEX_MASK			(1<<ENTITY_INDEX_BITS)-1
	
#define ENTITY_GENERATION_BITS		16
#define ENTITY_GENERATION_MASK		(1<<ENTITY_GENERATION_BITS)-1

#define MAX_ENTITY_COUNT			65535 // pow(2, ENTITY_INDEX_BITS) - 1
#define MAX_ENTITY_GENERATION		65535 // pow(2, ENTITY_GENERATION_BITS) - 1

#define ENTITY_IN_COMPONENT Entity parent; \
		inline uint32_t get_id() const {return parent.index();} \
		inline Entity get_entity() const {return parent;} 

#define ENTITY_IN_MULTICOMPONENT ENTITY_IN_COMPONENT uint32_t next;uint32_t prev;uint8_t num;

namespace EngineCore
{
	struct Entity
	{
	private:
		uint32_t id;
	public:
		inline uint32_t index() const {return id & ENTITY_INDEX_MASK;}
		inline uint32_t generation() const {return (id >> ENTITY_INDEX_BITS) & ENTITY_GENERATION_MASK;}
		inline void set(uint32_t idx, uint32_t gen)
		{id = (idx & ENTITY_INDEX_MASK) + ((gen & ENTITY_GENERATION_MASK) << ENTITY_INDEX_BITS);}
		inline bool isnull() const {return index() == MAX_ENTITY_COUNT && generation() == 0;}
		inline void setnull() {set(MAX_ENTITY_COUNT, 0);}

		friend bool operator==(const Entity& a, const Entity& b);
		friend Entity EntityFromUint(uint32_t a);
		friend uint32_t UintFromEntity(Entity e);
	};

	static bool operator==(const Entity& a, const Entity& b) { return a.id == b.id; }

	static bool EntIsEq(Entity a, Entity b) { return a == b; }

	inline static Entity EntityFromUint(uint32_t a) { Entity res; res.id = a; return res; }
	inline static uint32_t UintFromEntity(Entity e) { return e.id; }

	static void RegLuaClassEntity()
	{
		getGlobalNamespace(LSTATE)
			.addFunction("EntIsEq", &EntIsEq)
			.beginClass<Entity>("Entity")
				.addProperty("index", &Entity::index)
				.addProperty("generation", &Entity::generation)
				.addFunction("IsNull", &Entity::isnull)
				.addFunction("SetNull", &Entity::setnull)
			.endClass();
	}
}

	