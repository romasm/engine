#pragma once

#include "ECS_defines.h"
#include "Entity.h"

#define MAX_HIERARCHY_DEPTH 256

#define SCENEGRAPH_NULL_ID numeric_limits<uint32_t>::max()

#undef max

namespace EngineCore
{
	class SceneGraph
	{
		struct Relation
		{		
			uint32_t parentID; 
			uint32_t firstChildID;
			uint32_t nextID;
			uint32_t prevID;

			uint32_t nodeID;
			
			Relation()
			{
				parentID = SCENEGRAPH_NULL_ID;
				firstChildID = SCENEGRAPH_NULL_ID;
				nextID = SCENEGRAPH_NULL_ID;
				prevID = SCENEGRAPH_NULL_ID;
				nodeID = SCENEGRAPH_NULL_ID;
			}
			Relation(uint32_t nID)
			{
				parentID = SCENEGRAPH_NULL_ID;
				firstChildID = SCENEGRAPH_NULL_ID;
				nextID = SCENEGRAPH_NULL_ID;
				prevID = SCENEGRAPH_NULL_ID;
				nodeID = nID;
			}
		};

	public:
		SceneGraph(uint32_t maxCount);
		~SceneGraph()
		{
			_DELETE(attachments_map);
		}

		uint32_t AddNode();
		uint32_t CopyNode(uint32_t srcNodeID);
		void DeleteComponent(uint32_t nodeID);

		inline bool IsValidNode(uint32_t nodeID)
		{
			uint32_t& lookupID = lookup[nodeID];
			if( lookupID == SCENEGRAPH_NULL_ID )
				return false;
			return true;
		}

		inline uint32_t GetNodesCount() const {return dirty.size();}
		
		void Update();

		// TODO: prevent non-uniform scaling for parents
		// 1 - dont inherit scaling
		// 2 - inherit uniformed scaling
		inline void UpdateComponent(uint32_t& lookupID)
		{
			Relation& relt = relations[lookupID];
			if( relt.parentID != SCENEGRAPH_NULL_ID )
				worldTransformation[lookupID] = XMMatrixMultiply(localTransformation[lookupID], worldTransformation[relt.parentID]);
			else
				worldTransformation[lookupID] = localTransformation[lookupID];
			dirty[lookupID] = false;
		}

		void ForceUpdate(uint32_t nodeID)
		{
			uint32_t lookupID = lookup[nodeID];
			if(lookupID == SCENEGRAPH_NULL_ID)
				return;
			UpdateComponent(lookupID);
		}

		bool SetDirty(uint32_t nodeID);

		void PreLoad();
		bool PostLoadParentsResolve();
		
		bool Attach(uint32_t child, uint32_t parent);
		bool Detach(uint32_t child);
		bool DetachChildren(uint32_t parent);

		uint32_t GetParent(uint32_t child);
		uint32_t GetChildFirst(uint32_t parent);
		uint32_t GetChildNext(uint32_t child);

		inline bool SetTransformation(uint32_t nodeID, const XMMATRIX& matrix)
		{
			uint32_t& lookupID = lookup[nodeID];
			if( lookupID == SCENEGRAPH_NULL_ID )
				return false;
			localTransformation[lookupID] = matrix;
			dirty[lookupID] = true;
		}
		
		inline const XMMATRIX* GetLocalTransformation(uint32_t nodeID)
		{
			uint32_t& lookupID = lookup[nodeID];
			if( lookupID == SCENEGRAPH_NULL_ID )
				return nullptr;
			return &localTransformation[lookupID];
		}
		inline const XMMATRIX* GetWorldTransformation(uint32_t nodeID)
		{
			uint32_t& lookupID = lookup[nodeID];
			if( lookupID == SCENEGRAPH_NULL_ID )
				return nullptr;
			return &worldTransformation[lookupID];
		}
		
	private:
		inline void setDirty(uint32_t lookupID)
		{
			dirty[lookupID] = true;

			uint32_t child = relations[lookupID].firstChildID;
			while( child != SCENEGRAPH_NULL_ID )
			{
				setDirty(child);
				child = relations[child].nextID;
			}
		}

		inline void detach(Relation& childRelation, uint32_t childID)
		{
			if(childRelation.parentID == SCENEGRAPH_NULL_ID)
				return;
			
			auto& oldParentComp = relations[childRelation.parentID];
			if(oldParentComp.firstChildID == childID)
				oldParentComp.firstChildID = childRelation.nextID;
			if(childRelation.prevID != SCENEGRAPH_NULL_ID)
			{
				auto& prevComp = relations[childRelation.prevID];
				prevComp.nextID = childRelation.nextID;
			}
			if(childRelation.nextID != SCENEGRAPH_NULL_ID)
			{
				auto& nextComp = relations[childRelation.nextID];
				nextComp.prevID = childRelation.prevID;
			}
			childRelation.parentID = SCENEGRAPH_NULL_ID;
			childRelation.prevID = SCENEGRAPH_NULL_ID;
			childRelation.nextID = SCENEGRAPH_NULL_ID;	
		}

		inline void detachChildren(Relation& parentRelation)
		{			
			uint32_t child = parentRelation.firstChildID;
			while( child != SCENEGRAPH_NULL_ID )
			{
				auto& childRelation = relations[child];
				uint32_t next = childRelation.nextID;
				childRelation.parentID = SCENEGRAPH_NULL_ID;
				childRelation.prevID = SCENEGRAPH_NULL_ID;
				childRelation.nextID = SCENEGRAPH_NULL_ID;	
				child = next;
			}

			parentRelation.firstChildID = SCENEGRAPH_NULL_ID;
		}

		inline void movePrepare(uint32_t whatID, uint32_t whereID)
		{
			auto& whatComp = relations[whatID];
			if( whatComp.prevID != SCENEGRAPH_NULL_ID )
			{
				auto& prevComp = relations[whatComp.prevID];
				prevComp.nextID = whereID;
			}
			if( whatComp.nextID != SCENEGRAPH_NULL_ID )
			{
				auto& nextComp = relations[whatComp.nextID];
				nextComp.prevID = whereID;
			}
			if( whatComp.parentID != SCENEGRAPH_NULL_ID )
			{
				auto& parentComp = relations[whatComp.parentID];
				if(parentComp.firstChildID == whatID)
					parentComp.firstChildID = whereID;
			}

			uint32_t childID = whatComp.firstChildID;
			while( childID != SCENEGRAPH_NULL_ID )
			{
				auto& childComp = relations[childID];
				childComp.parentID = whereID;
				childID = childComp.nextID;
			}
			
		}

		// ~ 43 MB for 262144 nodes
		uint32_t capacity;

		RArray<bool> dirty;
		RArray<XMMATRIX> localTransformation;
		RArray<XMMATRIX> worldTransformation;
		RArray<Relation> relations; // 20 bytes

		RArray<uint32_t> lookup;
		RDeque<uint32_t> free_id;

		struct sort_data { int16_t hierarchy; uint32_t new_id; };
		RArray<sort_data> hierarchy_sort;
		RArray<uint32_t> links_fix;

		SArray<uint32_t, MAX_HIERARCHY_DEPTH> hi_buffer;

		// on load only
		unordered_map<uint32_t, string>* attachments_map;
		
		bool structureChanged;
	};
}