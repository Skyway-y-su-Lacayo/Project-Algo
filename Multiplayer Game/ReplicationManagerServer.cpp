#include "Networks.h"
#include "ReplicationManagerServer.h"

void ReplicationManagerServer::create(uint32 networkID) {
	actions.push_back(ReplicationCommand(networkID, ReplicationAction::CREATE));
}

void ReplicationManagerServer::destroy(uint32 networkID) {
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
