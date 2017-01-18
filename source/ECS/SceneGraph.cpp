#include "stdafx.h"
#include "SceneGraph.h"
#include "World.h"

using namespace EngineCore;

SceneGraph::SceneGraph(uint32_t maxCount, BaseWorld* w)
{	
	world = w;

	structureChanged = true;
	capacity = maxCount;
	
	dirty.create(capacity);
	localTransformation.create(capacity);
	worldTransformation.create(capacity);
	relations.create(capacity);

	lookup.create(capacity);
	lookup.resize(capacity);
	lookup.assign(SCENEGRAPH_NULL_ID);

	free_id.create(capacity);
	free_id.resize(capacity);
	for(uint32_t i = 0; i < capacity; i++)
		free_id[i] = i;

	hierarchy_sort.create(capacity);
	links_fix.create(capacity);

	entityForNode.create(capacity);
	entityForNode.resize(capacity);

	Entity e;
	e.setnull();
	entityForNode.assign(e);
}

uint32_t SceneGraph::AddNode(Entity e)
{
	if(free_id.size() == 0)
	{
		ERR("Can\'t add node, scene graph overflow!");
		return SCENEGRAPH_NULL_ID;
	}

	UINT nodeID = free_id.front();
	free_id.pop_front();

	lookup[nodeID] = (uint32_t)dirty.size();

	dirty.push_back(true);
	localTransformation.push_back(XMMatrixIdentity());			
	worldTransformation.push_back(XMMatrixIdentity());		
	relations.push_back(Relation(nodeID));	

	entityForNode[nodeID] = e;

	return nodeID;
}

uint32_t SceneGraph::CopyNode(uint32_t srcNodeID, Entity dest)
{
	uint32_t lookupSrcID = lookup[srcNodeID];
	if(lookupSrcID == SCENEGRAPH_NULL_ID)
		return SCENEGRAPH_NULL_ID;

	uint32_t resID = AddNode(dest);
	if(resID == SCENEGRAPH_NULL_ID)
		return SCENEGRAPH_NULL_ID;

	uint32_t& lookupDstID = lookup[resID];
	localTransformation[lookupDstID] = worldTransformation[lookupSrcID]; // bacase node is root
	worldTransformation[lookupDstID] = worldTransformation[lookupSrcID];
	return resID;
}

void SceneGraph::Update()
{
	if(structureChanged)
	{
		hierarchy_sort.resize(dirty.size());
		links_fix.resize(dirty.size());
		
		// build hierarchy params
		for(uint32_t i = 0; i < hierarchy_sort.size(); i++)
		{
			hierarchy_sort[i].new_id = i;
			hierarchy_sort[i].hierarchy = -1;
		}

		for(uint32_t i = 0; i < dirty.size(); i++)
		{
			if(hierarchy_sort[i].hierarchy >= 0)
				continue;
			
			uint32_t lookupID = i;
			Relation* relt = &relations[lookupID];

			hi_buffer.resize(0);
			hi_buffer.push_back(lookupID);

			while( relt->parentID != SCENEGRAPH_NULL_ID && hierarchy_sort[relt->parentID].hierarchy < 0 )
			{
				lookupID = relt->parentID;
				relt = &relations[lookupID];
				hi_buffer.push_back(lookupID);
			}
			
			if(hi_buffer.full())
				ERR("Scene hierarchy is too deep! Unpredictable behavior expected!");
			
			int16_t hi = 0;
			if(relt->parentID != SCENEGRAPH_NULL_ID)
				hi = hierarchy_sort[relt->parentID].hierarchy + 1;

			for(int32_t j = (int32_t)hi_buffer.size() - 1; j >= 0; j--)
			{
				hierarchy_sort[hi_buffer[j]].hierarchy = hi;
				hi++;
			}
		}

		// sorting IDs
		sort(hierarchy_sort.begin(), hierarchy_sort.end(), 
			[](const sort_data& a, const sort_data& b) -> bool { return a.hierarchy < b.hierarchy;});
		
		// reorder components
		for (uint32_t i = 0; i < dirty.size(); i++)
		{
			if (hierarchy_sort[i].hierarchy < 0)
				continue;
			
			uint32_t move_to = i;

			Relation temp_relt = relations[move_to];
			bool temp_dirty = dirty[move_to];
			XMMATRIX temp_local = localTransformation[move_to];
			XMMATRIX temp_world = worldTransformation[move_to];
						
			uint32_t move_from;
			while( (move_from = hierarchy_sort[move_to].new_id) != i )
			{
				hierarchy_sort[move_from].hierarchy = -1;

				relations[move_to] = relations[move_from];
				localTransformation[move_to] = localTransformation[move_from];
				worldTransformation[move_to] = worldTransformation[move_from];
				dirty[move_to] = dirty[move_from];
				lookup[relations[move_to].nodeID] = move_to;

				links_fix[move_from] = move_to;

				move_to = move_from;				
			}

			hierarchy_sort[move_from].hierarchy = -1;

			relations[move_to] = temp_relt;
			localTransformation[move_to] = temp_local;
			worldTransformation[move_to] = temp_world;
			dirty[move_to] = temp_dirty;
			lookup[relations[move_to].nodeID] = move_to;

			links_fix[i] = move_to;
		}

		// fix links
		for(uint32_t i = 0; i < relations.size(); i++)
		{
			if(relations[i].parentID != SCENEGRAPH_NULL_ID)
				relations[i].parentID = links_fix[relations[i].parentID];
			if(relations[i].firstChildID != SCENEGRAPH_NULL_ID)
				relations[i].firstChildID = links_fix[relations[i].firstChildID];
			if(relations[i].prevID != SCENEGRAPH_NULL_ID)
				relations[i].prevID = links_fix[relations[i].prevID];
			if(relations[i].nextID != SCENEGRAPH_NULL_ID)
				relations[i].nextID = links_fix[relations[i].nextID];
		}

		structureChanged = false;
	}

	for(uint32_t i = 0; i < dirty.size(); i++)
	{
		if(dirty[i])
			UpdateNode(i);
	}
}

void SceneGraph::DeleteNode(uint32_t nodeID)
{
	uint32_t lookupID = lookup[nodeID];
	if(lookupID == SCENEGRAPH_NULL_ID)
		return;
	Relation& relt = relations[lookupID];

	detach(relt, lookupID);
	detachChildren(relt);
	
	uint32_t lastID = (uint32_t)relations.size() - 1;
	if(lastID != lookupID)
		movePrepare(lastID, lookupID);

	uint32_t rmv_id = lookup[nodeID];
	if(rmv_id == SCENEGRAPH_NULL_ID)
		return;
		
	if(rmv_id != lastID)
	{
		uint32_t old_id = lastID;
		lookup[relations[old_id].nodeID] = rmv_id;

		structureChanged = true;
	}
	relations.erase_and_pop_back(rmv_id);
	localTransformation.erase_and_pop_back(rmv_id);
	worldTransformation.erase_and_pop_back(rmv_id);
	dirty.erase_and_pop_back((size_t)rmv_id);

	free_id.push_back(nodeID);
	lookup[nodeID] = SCENEGRAPH_NULL_ID;
	entityForNode[nodeID].setnull();
}

bool SceneGraph::IsDirty(uint32_t nodeID)
{
	uint32_t lookupID = lookup[nodeID];
	if(lookupID == SCENEGRAPH_NULL_ID)
		return false;

	return dirty[lookupID];
}

bool SceneGraph::SetDirty(uint32_t nodeID)
{
	uint32_t lookupID = lookup[nodeID];
	if(lookupID == SCENEGRAPH_NULL_ID)
		return false;

	dirty[lookupID] = true;
	Entity& e = entityForNode[nodeID];
	if(e.isnull())
	{
		ERR("Scene graph corruption: entityForNode");
		return false;
	}
	world->SetDirtyFromSceneGraph(e);

	uint32_t child = relations[lookupID].firstChildID;
	while( child != SCENEGRAPH_NULL_ID )
	{
		SetDirty(relations[child].nodeID);
		child = relations[child].nextID;
	}
	return true;
}

//		TODO: prevent loop attaching
bool SceneGraph::Attach(uint32_t child, uint32_t parent)
{
	uint32_t childLookupID = lookup[child];
	if( childLookupID == SCENEGRAPH_NULL_ID )
	{
		ERR("Attachable node does not exist!");
		return false;
	}

	if(parent == SCENEGRAPH_NULL_ID)
		return Detach(child);

	Relation& childRelt = relations[childLookupID];

	uint32_t parentLookupID = lookup[parent];
	if(parentLookupID == SCENEGRAPH_NULL_ID)
	{
		ERR("Node to attach to does not exist!");
		return false;
	}
	auto& parentRelt = relations[parentLookupID];

	detach(childRelt, childLookupID);
	
	childRelt.parentID = parentLookupID;

	if( parentRelt.firstChildID != SCENEGRAPH_NULL_ID )
	{
		uint32_t otherChildID = parentRelt.firstChildID;
		auto& otherChildComp = relations[otherChildID];
		while( otherChildComp.nextID != SCENEGRAPH_NULL_ID )
		{
			otherChildID = otherChildComp.nextID;
			otherChildComp = relations[otherChildID];
		}

		otherChildComp.nextID = childLookupID;
		childRelt.prevID = otherChildID;
	}
	else
	{
		parentRelt.firstChildID = childLookupID;
	}

	if(childLookupID != relations.size() - 1)
		structureChanged = true;
	return true;
}

bool SceneGraph::Detach(uint32_t child)
{
	uint32_t lookupID = lookup[child];
	if(lookupID == SCENEGRAPH_NULL_ID)
		return false;

	detach(relations[lookupID], lookupID);
	return true;
}

bool SceneGraph::DetachChildren(uint32_t parent)
{
	uint32_t lookupID = lookup[parent];
	if(lookupID == SCENEGRAPH_NULL_ID)
		return false;

	detachChildren(relations[lookupID]);
	return true;
}

uint32_t SceneGraph::GetParent(uint32_t child)
{
	uint32_t lookupID = lookup[child];
	if(lookupID == SCENEGRAPH_NULL_ID)
		return SCENEGRAPH_NULL_ID;
	
	Relation& relt = relations[lookupID];
	if(relt.parentID == SCENEGRAPH_NULL_ID)
		return SCENEGRAPH_NULL_ID;

	return relations[relt.parentID].nodeID;
}

uint32_t SceneGraph::GetChildFirst(uint32_t parent)
{
	uint32_t lookupID = lookup[parent];
	if(lookupID == SCENEGRAPH_NULL_ID)
		return SCENEGRAPH_NULL_ID;

	Relation& relt = relations[lookupID];
	if(relt.firstChildID == SCENEGRAPH_NULL_ID)
		return SCENEGRAPH_NULL_ID;

	return relations[relt.firstChildID].nodeID;
}

uint32_t SceneGraph::GetChildNext(uint32_t child)
{
	uint32_t lookupID = lookup[child];
	if(lookupID == SCENEGRAPH_NULL_ID)
		return SCENEGRAPH_NULL_ID;

	Relation& relt = relations[lookupID];
	if(relt.nextID == SCENEGRAPH_NULL_ID)
		return SCENEGRAPH_NULL_ID;

	return relations[relt.nextID].nodeID;
}