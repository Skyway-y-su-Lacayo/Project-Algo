#include "Networks.h"
#include "ReplicationManagerClient.h"

void ReplicationManagerClient::read(const InputMemoryStream & packet) {
	
	// Iterate serialized list and execute actions.

	while (packet.RemainingByteCount() > 0) {
		ReplicationAction action = ReplicationAction::NONE;
		uint32 networkID = 0;
		packet >> action ;
		packet >> networkID;

		switch (action) {
			case ReplicationAction::CREATE:
			{
				// Introduce tag to know which object to create
				uint32 tag = ObjectType::EMPTY;
				packet >> tag;
				spawnObject(tag);
				break;
			}
			case ReplicationAction::UPDATE:
			{
				GameObject* object = App->modLinkingContext->getNetworkGameObject(networkID);
				float x = 0; float y = 0; float angle = 0;
				packet >> x; packet >> y;
				packet >> angle;
				object->position.x = x; object->position.y = y;
				object->angle = angle;
				// Lucas(TODO): Make functions for easier serialization

				break;
			}
			case ReplicationAction::DESTROY:
			{
				GameObject* object = App->modLinkingContext->getNetworkGameObject(networkID);
				App->modLinkingContext->unregisterNetworkGameObject(object);
				Destroy(object);
				// Do nothing, networkID alone is enough
				break;
			}
		}
	}
}

void ReplicationManagerClient::spawnObject(int tag) {
	switch (tag) {
		case ObjectType::SPACESHIP:
		{
			App->modNetClient->spawnPlayer();
			break;
		}
		case ObjectType::LASER: 
		{
			break;
		}
	}
}
