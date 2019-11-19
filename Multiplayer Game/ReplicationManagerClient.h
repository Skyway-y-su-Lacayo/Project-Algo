#pragma once

#include"MemoryStream.h"
#include <vector>

class ReplicationManagerClient {
public:
	void read(const InputMemoryStream& packet);
	GameObject* spawnClientObject(int tag, uint32 networkID);

	void readPos(const InputMemoryStream& packet, GameObject* object);
};