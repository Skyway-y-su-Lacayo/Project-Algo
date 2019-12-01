#pragma once

#include "ModuleNetworking.h"

class ModuleNetworkingServer : public ModuleNetworking
{
public:

	//////////////////////////////////////////////////////////////////////
	// ModuleNetworkingServer public methods
	//////////////////////////////////////////////////////////////////////

	void setListenPort(int port);
	float getReplicationCadence() { return replicationDeliveryIntervalSeconds; };




private:

	//////////////////////////////////////////////////////////////////////
	// ModuleNetworking virtual methods
	//////////////////////////////////////////////////////////////////////

	bool isServer() const override { return true; }

	void onStart() override;

	void onGui() override;

	void onPacketReceived(const InputMemoryStream &packet, const sockaddr_in &fromAddress) override;

	void onUpdate() override;

	void onConnectionReset(const sockaddr_in &fromAddress) override;

	void onDisconnect() override;


	//////////////////////////////////////////////////////////////////////
	// Client proxies
	//////////////////////////////////////////////////////////////////////

	uint32 nextClientId = 0;

	struct ClientProxy
	{
		bool connected = false;
		sockaddr_in address;
		uint32 clientId;
		std::string name;
		GameObject *gameObject = nullptr;
		double lastPacketReceivedTime = 0.0f;
		float secondsSinceLastReplication = 0.0f;

		uint32 nextExpectedInputSequenceNumber = 0;
		InputController gamepad;
		MouseController mouse;

		Timer receivePingTimer;
		ReplicationManagerServer replicationManager;
		DeliveryManager deliveryManager;

		uint32 last_frame = 0;
	};



	ClientProxy * createClientProxy();

	ClientProxy * getClientProxy(const sockaddr_in &clientAddress);

	void destroyClientProxy(ClientProxy * proxy); // Destroys spaceship as well


	void sendPacketAll(OutputMemoryStream& data);


	uint32 connectedClients();


public:
	//I'm sorry for this -Lorién
	ClientProxy clientProxies[MAX_CLIENTS];
	//////////////////////////////////////////////////////////////////////
	// Spawning network objects
	//////////////////////////////////////////////////////////////////////

	GameObject * spawnPlayer(ClientProxy &clientProxy, uint8 type, uint8 team); // ObjectType enum
	GameObject * spawnPlayerShooter(ClientProxy &clientProxy, uint8 team);
	GameObject * spawnPlayerReflector(ClientProxy &clientProxy, uint8 team);

	GameObject * spawnReflectorBarrier(GameObject* parent);
	GameObject * spawnBullet(GameObject *parent, ColliderType col_type);

	bool checkSpaceshipAndTeam(uint8 type, uint8 team);

	// NOTE(jesus): Here go spawn methods for each type of network objects


	//////////////////////////////////////////////////////////////////////
	// Erase network gameobjects
	//////////////////////////////////////////////////////////////////////

	void destroyClientProxyByGO(GameObject* spaceship);
	void resetGame();


private:

	//////////////////////////////////////////////////////////////////////
	// Updating / destroying network objects
	//////////////////////////////////////////////////////////////////////

	void destroyNetworkObject(GameObject *gameObject);
	friend void (NetworkDestroy)(GameObject *);

	void updateNetworkObject(GameObject *gameObject);
	friend void (NetworkUpdate)(GameObject *);



	//////////////////////////////////////////////////////////////////////
	// State
	//////////////////////////////////////////////////////////////////////

	enum class ServerState
	{
		Stopped,
		Listening
	};

	ServerState state = ServerState::Stopped;

	uint16 listenPort = 0;
	
	float replicationDeliveryIntervalSeconds = 0.1f;
	Timer sendReplicationTimer;

	void manageSendReplication();

	

	// Timeout/ping

	void manageReceivePing(ClientProxy* clientProxy);
	void manageSendPing();
};


// NOTE(jesus): It marks an object for replication updates
void NetworkUpdate(GameObject *gameObject);

// NOTE(jesus): For network objects, use this version instead of
// the default Destroy(GameObject *gameObject) one. This one makes
// sure to notify the destruction of the object to all connected
// machines.
void NetworkDestroy(GameObject *gameObject);
