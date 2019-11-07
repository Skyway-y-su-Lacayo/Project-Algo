#pragma once

#include"ReplicationCommand.h"
#include"MemoryStream.h"
#include<vector>

class ReplicationManagerServer {
public:
	void create(uint32 networkID);
	void destroy(uint32 networkID);
	void update(uint32 networkID);


	void write(OutputMemoryStream& packet);

	std::vector<ReplicationCommand> actions;
};