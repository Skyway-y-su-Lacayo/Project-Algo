#include "Networks.h"


void ModuleLinkingContext::registerNetworkGameObject(GameObject *gameObject)
{
	for (uint16 i = 0; i < MAX_NETWORK_OBJECTS; ++i)
	{
		if (networkGameObjects[i] == nullptr)
		{
			gameObject->networkId = makeNetworkId(i);
			networkGameObjects[i] = gameObject;
			networkGameObjectsCount++;
			return;
		}
	}

	ASSERT(false); // NOTE(jesus): Increase MAX_NETWORKED_OBJECTS if necessary
}

void ModuleLinkingContext::registerNetworkGameObjectWithNetworkId(GameObject * gameObject, uint32 networkId)
{
	uint16 arrayIndex = arrayIndexFromNetworkId(networkId);
	ASSERT(arrayIndex < MAX_NETWORK_OBJECTS);
	ASSERT(networkGameObjects[arrayIndex] == nullptr);
	networkGameObjects[arrayIndex] = gameObject;
	gameObject->networkId = networkId;
	networkGameObjectsCount++;
}

GameObject * ModuleLinkingContext::getNetworkGameObject(uint32 networkId)
{
	uint16 arrayIndex = arrayIndexFromNetworkId(networkId);
	ASSERT(arrayIndex < MAX_NETWORK_OBJECTS);

	GameObject *gameObject = networkGameObjects[arrayIndex];

	if (gameObject != nullptr && gameObject->networkId == networkId)
	{
		return gameObject;
	}
	else
	{
		return nullptr;
	}
}

void ModuleLinkingContext::getNetworkGameObjects(GameObject * gameObjects[MAX_NETWORK_OBJECTS], uint16 * count)
{
	uint16 insertIndex = 0;

	for (uint16 i = 0; i < MAX_NETWORK_OBJECTS && insertIndex < networkGameObjectsCount; ++i)
	{
		if (networkGameObjects[i] != nullptr)
		{
			gameObjects[insertIndex++] = networkGameObjects[i];
		}
	}

	*count = insertIndex;
}

uint16 ModuleLinkingContext::getNetworkGameObjectsCount() const
{
	return networkGameObjectsCount;
}

void ModuleLinkingContext::unregisterNetworkGameObject(GameObject *gameObject)
{
	uint16 arrayIndex = arrayIndexFromNetworkId(gameObject->networkId);
	ASSERT(arrayIndex < MAX_NETWORK_OBJECTS);
	if (networkGameObjects[arrayIndex] == gameObject)
	{
		networkGameObjects[arrayIndex] = nullptr;
		gameObject->networkId = 0;
		networkGameObjectsCount--;
	}
}

void ModuleLinkingContext::clear()
{
	for (uint16 i = 0; i < MAX_NETWORK_OBJECTS; ++i)
	{
		if (networkGameObjects[i] != nullptr)
		{
			networkGameObjects[i]->networkId = 0;
			networkGameObjects[i] = nullptr;
		}
	}
	networkGameObjectsCount = 0;
}

uint32 ModuleLinkingContext::makeNetworkId(uint16 arrayIndex)
{
	uint32 magicNumber = nextMagicNumber++;
	uint32 networkId = (magicNumber << 16) | arrayIndex;
	return networkId;
}

uint16 ModuleLinkingContext::arrayIndexFromNetworkId(uint32 networkId)
{
	uint16 arrayIndex = networkId & 0xffff;
	return arrayIndex;
}
