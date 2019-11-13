#pragma once

#include"MemoryStream.h"
#include <vector>

class ReplicationManagerClient {
public:
	void read(const InputMemoryStream& packet);
	void spawnClientObject(int tag, uint32 networkID);
};