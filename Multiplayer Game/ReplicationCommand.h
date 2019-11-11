#pragma once


enum ReplicationAction {
	NONE,
	CREATE,
	DESTROY,
	UPDATE,
	CREATE_EXISTING
};

struct ReplicationCommand {
public:

	ReplicationCommand(uint32 networkID, ReplicationAction action) : networkID(networkID), action(action) {}
	bool operator ==(ReplicationCommand const & r_c) {
		return networkID == r_c.networkID && action == r_c.action;
	}
	uint32 networkID = -1;
	ReplicationAction action = NONE;

};

