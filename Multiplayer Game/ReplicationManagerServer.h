#pragma once

#include"ReplicationCommand.h"
#include"MemoryStream.h"
#include<vector>

class Delivery;
class ReplicationManagerServer {
public:

	void create(uint32 networkID);
	void destroy(uint32 networkID);
	void update(uint32 networkID);


	void write(OutputMemoryStream& packet, Delivery* delivery = nullptr, std::vector<ReplicationCommand> _actions = std::vector<ReplicationCommand>());


	// The index to access the array is the networkID of the object (the index part)
	std::vector<ReplicationCommand> actions;
	void writePos(OutputMemoryStream& packet, GameObject* object);

};