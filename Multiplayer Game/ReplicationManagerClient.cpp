 #include "Networks.h"
#include "ReplicationManagerClient.h"

void ReplicationManagerClient::read(const InputMemoryStream & packet) {
	
	// Iterate serialized list and execute actions.
	//If i do this, it crashes, wtf?
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
				uint32 team = ObjectTeam::TEAM_1;
				packet >> tag;
				packet >> team;
				if (!App->modLinkingContext->IsSlotCorrect(networkID)){
					// Delete object which is in the slot (destroy has been lost)
					GameObject* object = App->modLinkingContext->getNetworkGameObjectBySlot(networkID);
					App->modLinkingContext->unregisterNetworkGameObject(object);
					Destroy(object);
					ELOG("Assuming %i gameObject needs to be deleted");
				}
				GameObject* created = spawnClientObject(networkID, tag, team);
				readInitialPos(packet, created);
				break;
			}
			case ReplicationAction::UPDATE:
			{
				// This will crash if the object is a nullptr
				GameObject* object = App->modLinkingContext->getNetworkGameObject(networkID);
				if(!interpolationUpdate(packet, object))
					ELOG("Trying to update an unexisting NetworkID: %i", networkID);


				break;
			}
			case ReplicationAction::DESTROY:
			{
				
				GameObject* object = App->modLinkingContext->getNetworkGameObject(networkID);
				if(object){
					App->modLinkingContext->unregisterNetworkGameObject(object);
					Destroy(object);
				}
				else // A new create has arrived so the object was already deleted
					ELOG("Gameobject assigned to NetworkID %i doesn not exist, can't destroy", networkID);
				break;
			}
		}
	}
}

GameObject* ReplicationManagerClient::spawnClientObject(uint32 networkID, uint32 tag, uint32 team) {

	GameObject* ret = nullptr;

	switch (tag) {
		case ObjectType::SHOOTER:
		case ObjectType::REFLECTOR: 
		{
			// Spawn shooter/reflector
			ret = App->modNetClient->spawnPlayer(networkID, tag, team);
			break;
		}
		case ObjectType::REFLECTOR_BARRIER: 
		{
			ret = App->modNetClient->spawnReflectorBarrier(networkID, team);
			break;
		}
		case ObjectType::SOFT_LASER: 
		case ObjectType::HARD_LASER:
		{
			ret = App->modNetClient->spawnBullet(networkID, tag, team);

			break;
		}
	}

	return ret;
}

bool  ReplicationManagerClient::readInitialPos(const InputMemoryStream & packet, GameObject * object) {
	bool ret = false;
	packet >> object->position.x; packet >> object->position.y;
	packet >> object->angle;

	if (object) {
		ret = true;
		object->interpolationCreate();
	}
	return ret;
}


bool ReplicationManagerClient::interpolationUpdate(const InputMemoryStream & packet, GameObject * object)
{
	bool ret = false;
	vec2 new_pos;
	float angle;
	packet >> new_pos.x; packet >> new_pos.y;
	packet >> angle;
	uint16 lives;
	packet >> lives;

	if (object) {
		ret = true;
		if (object->tag == ObjectType::REFLECTOR || object->tag == ObjectType::SHOOTER) {
			Player* behaviour = (Player*)object->behaviour;
			behaviour->lives = lives;
		}

		object->interpolationUpdate(new_pos, angle);
	}

	return ret;
}


