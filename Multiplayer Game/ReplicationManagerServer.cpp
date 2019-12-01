#include "Networks.h"
#include "ReplicationManagerServer.h"

#include <list>
void ReplicationManagerServer::create(uint32 networkID) {
	actions.push_back(ReplicationCommand(networkID, ReplicationAction::CREATE));
}

void ReplicationManagerServer::destroy(uint32 networkID) {
	actions.push_back(ReplicationCommand(networkID, ReplicationAction::DESTROY));
}

void ReplicationManagerServer::update(uint32 networkID) {
	actions.push_back(ReplicationCommand(networkID, ReplicationAction::UPDATE));

}

void ReplicationManagerServer::write(OutputMemoryStream & packet, Delivery* delivery, std::vector<ReplicationCommand> _actions) {
	// Iterate actions and serialize object fields when needed.
	if (ReplicationDelegate* tmp = (ReplicationDelegate*)delivery->delegate)
	{
		tmp->actions = actions;
	}
	
	if (_actions.size() != 0)
		actions = _actions;

	for (auto action : actions) {
		// Action and network id commun to all

		if ((action.action == ReplicationAction::UPDATE || action.action == ReplicationAction::CREATE) && !App->modLinkingContext->getNetworkGameObject(action.networkID))
			continue; // Don't update/create if the object doesn't exist anymore

		packet << action.action;
		packet << action.networkID;
		switch (action.action) {
			case ReplicationAction::CREATE:
			{
				// Introduce tag to know which object to create
				
				GameObject* object = App->modLinkingContext->getNetworkGameObject(action.networkID);

				packet << object->tag;
				packet << object->team;
				writePos(packet, object);
				break;
			}
			case ReplicationAction::UPDATE:
			{
				GameObject* object = App->modLinkingContext->getNetworkGameObject(action.networkID);
				writePos(packet, object);
				Player* behaviour = (Player*)object->behaviour;
				packet << behaviour->lives;
				break;
			}
			case ReplicationAction::DESTROY:
			{
				// Do nothing, networkID alone is enough
				break;
			}
		}
	}

	actions.clear();


}

void ReplicationManagerServer::writePos(OutputMemoryStream & packet, GameObject * object) {
	packet << object->position.x; packet << object->position.y;
	packet << object->angle;
}

void ReplicationManagerServer::ValidateActions(std::vector<ReplicationCommand>* actions)
{
	for (auto item = actions->begin(); item != actions->end(); item++)
	{
		GameObject* object = App->modLinkingContext->getNetworkGameObject((*item).networkID);
		if(!object)
		{
			item = actions->erase(item);
			if (item == actions->end())
				break;
		}
	}
}
