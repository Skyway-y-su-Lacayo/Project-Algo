#pragma once
class DeliveryManager;
class ModuleNetworkingServer;

#define MS_TO_DELIVERY_TIMEOUT 1000
//TODO: WHAT IS THIS?
class ReplicationCommand;

class DeliveryDelegate
{
public:
	virtual void onDeliverySuccess(DeliveryManager* deliveryManager) = 0;
	virtual void onDeliveryFailure(DeliveryManager* deliveryManager) = 0;
};

class ReplicationDelegate : public DeliveryDelegate
{

public:
	ModuleNetworkingServer* networking_server;

	void onDeliverySuccess(DeliveryManager* deliveryManager)
	{}
	void onDeliveryFailure(DeliveryManager* deliveryManager);


};

struct Delivery
{
	uint32 sequenceNumber = 0;
	Timer timer;
	bool to_remove = false;
	std::vector<ReplicationCommand> actions;
	DeliveryDelegate* delegate = nullptr;
};

class DeliveryManager
{
public:
	DeliveryManager();
	~DeliveryManager();

	Delivery* writeSequenceNumber(OutputMemoryStream& packet);

	bool processSequenceNumber(const InputMemoryStream& packet);

	bool hasSequenceNumbersPendingAck()const;
	void writeSequenceNumbersPendingAck(OutputMemoryStream& packet);

	void processAckdSequenceNumbers(const InputMemoryStream& packet);
	void processTimedOutPackets();

	void clear();

	uint32 seq_number = 0;

	std::vector<Delivery*> pending_deliveries;
	std::vector<uint32> pending_ack;

private:



};


