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
				GameObject* created = spawnClientObject(tag, networkID);
				readInitialPos(packet, created);
				break;
			}
			case ReplicationAction::UPDATE:
			{
				// This will crash if the object is a nullptr
				if (GameObject* object = App->modLinkingContext->getNetworkGameObject(networkID)) 
					interpolationUpdate(packet, object);
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

GameObject* ReplicationManagerClient::spawnClientObject(int tag, uint32 networkID) {

	GameObject* ret = nullptr;

	switch (tag) {
		case ObjectType::SHOOTER:
		case ObjectType::REFLECTOR: 
		{
			// Spawn shooter/reflector
			ret = App->modNetClient->spawnPlayer(networkID, tag);
			break;
		}
		case ObjectType::REFLECTOR_BARRIER: 
		{
			ret = App->modNetClient->spawnReflector(networkID);
			break;
		}
		case ObjectType::SOFT_LASER: 
		case ObjectType::HARD_LASER:
		{
			ret = App->modNetClient->spawnBullet(networkID, tag);

			break;
		}
	}

	return ret;
}

void ReplicationManagerClient::readInitialPos(const InputMemoryStream & packet, GameObject * object) {
	packet >> object->position.x; packet >> object->position.y;
	packet >> object->angle;

	object->interpolationCreate();
}

void ReplicationManagerClient::interpolationUpdate(const InputMemoryStream & packet, GameObject * object)
{
	vec2 new_pos;
	float angle;
	packet >> new_pos.x; packet >> new_pos.y;
	packet >> angle;

	object->interpolationUpdate(new_pos, angle);
}


