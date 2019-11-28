#include "Networks.h"
#include "ModuleGameObject.h"

void GameObject::releaseComponents()
{
	if (behaviour != nullptr)
	{
		delete behaviour;
		behaviour = nullptr;
	}
	if (collider != nullptr)
	{
		App->modCollision->removeCollider(collider);
		collider = nullptr;
	}
}

bool ModuleGameObject::init()
{
	return true;
}

bool ModuleGameObject::preUpdate()
{
	for (GameObject &gameObject : gameObjects)
	{
		if (gameObject.state == GameObject::NON_EXISTING) continue;

		if (gameObject.state == GameObject::DESTROYING)
		{
			gameObject.releaseComponents();
			gameObject = GameObject();
			gameObject.state = GameObject::NON_EXISTING;
		}
		else if (gameObject.state == GameObject::CREATING)
		{
			if (gameObject.behaviour != nullptr)
				gameObject.behaviour->start();
			gameObject.state = GameObject::UPDATING;
		}
	}

	return true;
}

bool ModuleGameObject::update()
{
	for (GameObject &gameObject : gameObjects)
	{
		if (gameObject.state == GameObject::UPDATING)
		{
			if (gameObject.behaviour != nullptr)
				gameObject.behaviour->update();
		}
	}

	return true;
}

bool ModuleGameObject::postUpdate()
{
	return true;
}

bool ModuleGameObject::cleanUp()
{
	for (GameObject &gameObject : gameObjects)
	{
		gameObject.releaseComponents();
	}

	return true;
}

GameObject * ModuleGameObject::Instantiate()
{
	for (GameObject &gameObject : App->modGameObject->gameObjects)
	{
		if (gameObject.state == GameObject::NON_EXISTING)
		{
			gameObject.state = GameObject::CREATING;
			return &gameObject;
		}
	}

	ASSERT(0); // NOTE(jesus): You need to increase MAX_GAME_OBJECTS in case this assert crashes
	return nullptr;
}

void ModuleGameObject::Destroy(GameObject * gameObject)
{
	ASSERT(gameObject->networkId == 0); // NOTE(jesus): If it has a network identity, it must be destroyed by the Networking module first
	
	gameObject->state = GameObject::DESTROYING;
}

void ModuleGameObject::calculateInterpolation(uint32 not_update)
{
	uint32 reflector_id = 0;
	GameObject* curr_player = GetGameObejctFromNetworkID(not_update);
	if(curr_player)
		if (curr_player->tag == REFLECTOR)
		{
			ReflectorClient* behaviour = (ReflectorClient*)curr_player->behaviour;
			if(GameObject* barrier = behaviour->reflector_barrier)
				reflector_id = barrier->networkId;
		}


	for (GameObject &gameObject : App->modGameObject->gameObjects)
	{
		if (gameObject.state == GameObject::NON_EXISTING  || gameObject.networkId == not_update || gameObject.networkId == reflector_id || gameObject.networkId == 0)
			continue;

		float interpolation_coeficient = gameObject.timeSinceLastUpdate.ReadSeconds() / App->modNetServer->getReplicationCadence();
		vec2 delta_pos = (gameObject.final_pos - gameObject.initial_pos) * interpolation_coeficient;

		float shortest_angle = ((((int)(gameObject.final_angle - gameObject.initial_angle) % 360) + 540) % 360) - 180;
		float delta_angle = (int)(shortest_angle * interpolation_coeficient) % 360;

		gameObject.position = gameObject.initial_pos + delta_pos;
		gameObject.angle = gameObject.initial_angle + delta_angle;
	}
}

GameObject * ModuleGameObject::GetGameObejctFromNetworkID(uint32 id)
{
	GameObject* ret = nullptr;
	for (GameObject &gameObject : App->modGameObject->gameObjects)
	{
		if (gameObject.networkId == id)
			ret = &gameObject;
	}
	return ret;
}

GameObject * ModuleGameObject::spawnBackground(vec2 position, vec2 size) {

	// Instanciate boundaries
	GameObject* background = Instantiate();
	background->texture = App->modResources->background;
	background->position = position;
	background->size = size;
	background->order = -1;
	return background;
}

GameObject * Instantiate()
{
	GameObject *result = ModuleGameObject::Instantiate();
	return result;
}

void Destroy(GameObject * gameObject)
{
	ModuleGameObject::Destroy(gameObject);
}
