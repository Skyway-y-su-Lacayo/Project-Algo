#pragma once

#include "Timer.h"

#define SIMULATE_REAL_WORLD_CONDITIONS

class ModuleNetworking : public Module
{
public:

	virtual bool isServer() const { return false; }

	virtual bool isClient() const { return false; }

	bool isConnected() const { return socket != INVALID_SOCKET; }

	void disconnect();



protected:

	//////////////////////////////////////////////////////////////////////
	// ModuleNetworking protected methods
	//////////////////////////////////////////////////////////////////////

	SOCKET socket = INVALID_SOCKET;

	bool createSocket();

	bool bindSocketToPort(int port);

	void sendPacket(const OutputMemoryStream &packet, const sockaddr_in &destAddress);

	void sendPacket(const char *data, uint32 size, const sockaddr_in &destAddress);

	void reportError(const char *message);

	//////////////////////////////////////////////////////////////////////
	// Ping variables
	//////////////////////////////////////////////////////////////////////


	Timer sendPingTimer;
	int pingsReceived = 0;
	bool blockPingsSend = false;

private:

	//////////////////////////////////////////////////////////////////////
	// Module virtual methods
	//////////////////////////////////////////////////////////////////////

	bool init() override;

	bool start() override;

	bool preUpdate() override;

	bool update() override;

	bool gui() override;

	bool stop() override;

	bool cleanUp() override;



	//////////////////////////////////////////////////////////////////////
	// ModuleNetworking methods
	//////////////////////////////////////////////////////////////////////

	uint32 sentPacketsCount = 0;
	uint32 receivedPacketsCount = 0;
	InputMemoryStream inPacket;

	void processIncomingPackets();

	virtual void onStart() = 0;

	virtual void onGui() = 0;

	virtual void onPacketReceived(const InputMemoryStream &packet, const sockaddr_in &fromAddress) = 0;

	virtual void onUpdate() = 0;

	virtual void onConnectionReset(const sockaddr_in &fromAddress) = 0;

	virtual void onDisconnect() = 0;



private:

#if defined(SIMULATE_REAL_WORLD_CONDITIONS)

	//////////////////////////////////////////////////////////////////////
	// Real world conditions simulation
	//////////////////////////////////////////////////////////////////////

	bool simulateLatency = false;
	float simulatedLatency = 0.07f;
	float simulatedJitter = 0.03f;
	bool simulateDrops = false;
	float simulatedDropRatio = 0.01f;
	uint32 simulatedPacketsReceived = 0;
	uint32 simulatedPacketsDropped = 0;

	void simulatedRealWorldConditions_EnqueuePacket(const InputMemoryStream &packet, const sockaddr_in &fromAddress);

	void simulatedRealWorldConditions_ProcessQueuedPackets();

	struct SimulatedPacket {
		InputMemoryStream packet;
		sockaddr_in fromAddress;
		double receptionTime;
	};

	std::deque<SimulatedPacket> simulatedPacketQueue;
	RandomNumberGenerator simulatedRandom;

#endif

};

