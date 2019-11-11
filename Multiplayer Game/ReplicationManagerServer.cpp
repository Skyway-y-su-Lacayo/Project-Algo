#include "Networks.h"
#include "ReplicationManagerServer.h"

#include <list>
void ReplicationManagerServer::create(uint32 networkID) {
	actions.push_back(ReplicationCommand(networkID, ReplicationAction::CREATE));
}

void ReplicationManagerServer::destroy(uint32 networkID) {

	// Erase possible events of update
	
	// Fill "actions to delete" list
	std::vector<ReplicationCommand> actionsToDelete;
	for (auto it = actions.begin(); it != actions.end(); it++) 
		if ((*it).networkID == networkID && (*it).action == ReplicationAction::UPDATE)
			actionsToDelete.push_back(*it);
	
	// Erase "actions to delete" from actions
	for (auto it = actionsToDelete.begin(); it != actionsToDelete.end(); it++)
		actions.erase(std::remove(actions.begin(), actions.end(), (*it)), actions.end());
		
	actions.push_back(ReplicationCommand(networkID, ReplicationAction::DESTROY));
}

void ReplicationManagerServer::update(uint32 networkID) {
	actions.push_back(ReplicationCommand(networkID, ReplicationAction::UPDATE));
}

void ReplicationManagerServer::write(OutputMemoryStream & packet) {
	// Iterate actions and serialize object fields when needed.
	packet << ServerMessage::Replication;

	for (auto action : actions) {
		// Action and network id commun to all
		packet << action.action;
		packet << action.networkID;
		switch (action.action) {
			case ReplicationAction::CREATE:
			{
				// Introduce tag to know which object to create
				
				GameObject* object = App->modLinkingContext->getNetworkGameObject(action.networkID);
				packet << object->tag;
				break;
			}
			case ReplicationAction::UPDATE:
			{
				GameObject* object = App->modLinkingContext->getNetworkGameObject(action.networkID);

				// In case an object has been destroyed during the replication interval

				packet << object->position.x; packet << object->position.y;
				packet << object->angle;

				break;
			}
			case ReplicationAction::DESTROY:
			{
				// Do nothing, networkID alone is enough
				break;
			}
		}
	}

	// Clear actions list after sending packet
	actions.clear();

}
