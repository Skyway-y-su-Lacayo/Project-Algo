#pragma once


enum ReplicationAction {
	NONE,
	CREATE,
	DESTROY,
	UPDATE
};

struct ReplicationCommand {
public:

	ReplicationCommand(uint32 networkID, ReplicationAction action) : networkID(networkID), action(action) {}
	uint32 networkID = -1;
	ReplicationAction action = NONE;

};

