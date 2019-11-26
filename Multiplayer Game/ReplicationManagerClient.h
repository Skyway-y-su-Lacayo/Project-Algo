#pragma once

#include"MemoryStream.h"
#include <vector>

class ReplicationManagerClient {
public:
	void read(const InputMemoryStream& packet);
	GameObject* spawnClientObject(uint32 tag, uint32 team, uint32 networkID);

	bool readInitialPos(const InputMemoryStream& packet, GameObject* object);
	bool interpolationUpdate(const InputMemoryStream& packet, GameObject* object);
};