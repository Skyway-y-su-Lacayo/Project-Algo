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
				spawnClientObject(tag, networkID);
				break;
			}
			case ReplicationAction::UPDATE:
			{
				// Lucas(TODO): Make functions for easier serialization
				float x = 0; float y = 0; float angle = 0;
				packet >> x; packet >> y;
				packet >> angle;
				
				if (GameObject* object = App->modLinkingContext->getNetworkGameObject(networkID))
				{
					object->position.x = x; object->position.y = y;
					object->angle = angle;
				}
				else
					ELOG("Gameobject assigned to NetworkID %i doesn not exist, can't update", networkID); 

				break;
			}
			case ReplicationAction::DESTROY:
			{
				GameObject* object = App->modLinkingContext->getNetworkGameObject(networkID);
				if(object){
					App->modLinkingContext->unregisterNetworkGameObject(object);
					Destroy(object);
				}
				else
					ELOG("Gameobject assigned to NetworkID %i doesn not exist, can't destroy", networkID);
				break;
			}
		}
	}
}

void ReplicationManagerClient::spawnClientObject(int tag, uint32 networkID) {
	switch (tag) {
		case ObjectType::SHOOTER:
		{
			//Spawn reflector
			App->modNetClient->spawnPlayer(networkID, tag);
			break;
		}
		case ObjectType::REFLECTOR: 
		{
			// Spawn reflector
			App->modNetClient->spawnPlayer(networkID, tag);
			break;
		}
		case ObjectType::LASER: 
		{
			App->modNetClient->spawnBullet(networkID);

			break;
		}
	}
}


