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
	for (GameObject &gameObject : App->modGameObject->gameObjects)
	{
		if (gameObject.state == GameObject::NON_EXISTING || gameObject.networkId == 0 || gameObject.networkId == not_update) 
			continue;

		float interpolation_coeficient = gameObject.timeSinceLastUpdate.ReadSeconds() / App->modNetServer->getReplicationCadence();
		vec2 delta_pos = (gameObject.final_pos - gameObject.initial_pos) * interpolation_coeficient;
		float delta_angle = (gameObject.final_angle - gameObject.initial_angle) * interpolation_coeficient;

		gameObject.position = gameObject.initial_pos + delta_pos;
		gameObject.angle = gameObject.initial_angle + delta_angle;
	}
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
