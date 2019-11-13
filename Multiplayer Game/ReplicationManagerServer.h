#pragma once

#include"ReplicationCommand.h"
#include"MemoryStream.h"
#include<vector>
class ReplicationManagerServer {
public:

	ReplicationManagerServer() {
		clearArray();
	}
	void create(uint32 networkID);
	void destroy(uint32 networkID);
	void update(uint32 networkID);


	void write(OutputMemoryStream& packet);

	void clearArray();

	// The index to access the array is the networkID of the object (the index part)
	ReplicationCommand actions[MAX_NETWORK_OBJECTS];
};