#pragma once

#include"MemoryStream.h"
#include <vector>

class ReplicationManagerClient {
public:
	void read(const InputMemoryStream& packet);
	void spawnObject(int tag);
	void spawnExistinObject(int tag, uint32 networkID);
};