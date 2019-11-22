#pragma once

#include"MemoryStream.h"
#include <vector>

class ReplicationManagerClient {
public:
	void read(const InputMemoryStream& packet);
	GameObject* spawnClientObject(int tag, uint32 networkID);

	void readInitialPos(const InputMemoryStream& packet, GameObject* object);
	void interpolationUpdate(const InputMemoryStream& packet, GameObject* object);
};