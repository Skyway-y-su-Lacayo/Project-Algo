#pragma once

#include "ModuleNetworking.h"
#include <map>

class ModuleNetworkingClient : public ModuleNetworking
{
public:

	//////////////////////////////////////////////////////////////////////
	// ModuleNetworkingClient public methods
	//////////////////////////////////////////////////////////////////////

	void setServerAddress(const char *serverAddress, uint16 serverPort);

	void setPlayerInfo(const char *playerName, uint8 spaceshipType, uint8 team);



	GameObject* spawnPlayer(uint32 networkID, uint8 type, uint8 team);
	GameObject* spawnTeamTag(GameObject* player);
	GameObject* spawnReflectorBarrier(uint32 networkID, uint8 team);
	GameObject* spawnBullet(uint32 networkID, uint8 type, uint8 team);


private:

	//////////////////////////////////////////////////////////////////////
	// ModuleNetworking virtual methods
	//////////////////////////////////////////////////////////////////////

	bool isClient() const override { return true; }

	void onStart() override;

	void onGui() override;

	void onPacketReceived(const InputMemoryStream &packet, const sockaddr_in &fromAddress) override;

	void onUpdate() override;

	void onConnectionReset(const sockaddr_in &fromAddress) override;

	void onDisconnect() override;




	//////////////////////////////////////////////////////////////////////
	// Client state
	//////////////////////////////////////////////////////////////////////

	enum class ClientState
	{
		Stopped,
		Start,
		WaitingWelcome,
		Playing
	};

	void clientSidePrediction(GameObject* go);

	ClientState state = ClientState::Stopped;

	std::string serverAddressStr;
	uint16 serverPort = 0;

	sockaddr_in serverAddress = {};
	std::string playerName = "player";
	uint8 spaceshipType = 0;
	uint8 team = 0;

	uint32 playerId = 0;
	uint32 networkId = 0;

	// ClientSide Prediction
	bool clientside_prediction = true;
	uint32 current_frame = 0;

	// Input ///////////

	static const int MAX_INPUT_DATA_SIMULTANEOUS_PACKETS = 64;

	// Queue of input data
	InputPacketData inputData[MAX_INPUT_DATA_SIMULTANEOUS_PACKETS];
	uint32 inputDataFront = 0;
	uint32 inputDataBack = 0;

	uint32 last_server_frame = 0;
	std::vector<InputPacketData> previous_inputs;

	float inputDeliveryIntervalSeconds = 0.05f;
	float secondsSinceLastInputDelivery = 0.0f;


	// Timeout / ping

	double lastPacketReceivedTime = 0.0f; // NOTE(jesus): Use this to implement client timeout
	Timer receivePingTimer;

	void initializePing();
	void managePing(sockaddr_in otherAddress);


	// Replication
	ReplicationManagerClient replicationClient;
	DeliveryManager deliveryManager;


	// Waiting for players
	bool waitingForPlayers = true;
	GameObject* waitingLabel = nullptr;

	// TeamLabels
	int teamTagsCount = 0;
	GameObject* teamTags[4] = { nullptr };

};

