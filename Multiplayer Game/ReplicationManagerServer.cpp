#include "Networks.h"
#include "ReplicationManagerServer.h"

#include <list>
void ReplicationManagerServer::create(uint32 networkID) {
	uint16 arrayIndex = networkID & 0xffff;

	// Let the object be destroyed before creating a new one, just ignore the command TODO(Lucas): Look for a new networkID for the obejct or something
	if(actions[arrayIndex].action != ReplicationAction::DESTROY)
		actions[arrayIndex] = ReplicationCommand(networkID, ReplicationAction::CREATE);
}

void ReplicationManagerServer::destroy(uint32 networkID) {

	// Erase possible events of update
	uint16 arrayIndex = networkID & 0xffff;

	// If there is no create in place, we can destroy
	if (actions[arrayIndex].action != ReplicationAction::CREATE)
		actions[arrayIndex] = ReplicationCommand(networkID, ReplicationAction::DESTROY);
	// If it is, we need to erase the create command
	else
		actions[arrayIndex] = ReplicationCommand();
}

void ReplicationManagerServer::update(uint32 networkID) {
	uint16 arrayIndex = networkID & 0xffff;

	// If there is no create or destroy event in place we can update
	if(actions[arrayIndex].action != ReplicationAction::CREATE && actions[arrayIndex].action != ReplicationAction::DESTROY)
		actions[arrayIndex] = ReplicationCommand(networkID, ReplicationAction::UPDATE);

}

void ReplicationManagerServer::write(OutputMemoryStream & packet) {
	// Iterate actions and serialize object fields when needed.
	packet << ServerMessage::Replication;

	for (auto action : actions) {
		// Action and network id commun to all

		if (action.action == ReplicationAction::NONE)
			continue;

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
	clearArray();

}

void ReplicationManagerServer::clearArray() {
	for (int i = 0; i < MAX_NETWORK_OBJECTS; i++)
		actions[i] = ReplicationCommand();
}
