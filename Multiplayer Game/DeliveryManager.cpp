#include "Networks.h"
#include "DeliveryManager.h"



DeliveryManager::DeliveryManager()
{
}


DeliveryManager::~DeliveryManager()
{
}

Delivery * DeliveryManager::writeSequenceNumber(OutputMemoryStream & packet)
{
	Delivery* ret = new Delivery;
	packet << seq_number;
	ret->sequenceNumber = seq_number;
	ret->timer.Start();
	pending_deliveries.push_back(ret);

	seq_number++;

	return ret;
}

bool DeliveryManager::processSequenceNumber(const InputMemoryStream & packet)
{
	//TODO check what out of order means
	bool ret = false;

	uint32 current_seq;
	packet >> current_seq;
	pending_ack.push_back(current_seq);

	
	if (current_seq <= seq_number)
		ret = true;
	else
	{
		ELOG("A packet was discarded because the sequence number was out of order");
		ELOG("Incoming Seq number: %lu, Own Seq number: %lu", current_seq, seq_number);
	}


	seq_number++;
	return ret;
}

void DeliveryManager::writeSequenceNumbersPendingAck(OutputMemoryStream & packet)
{
	packet << pending_ack;
	pending_ack.clear();
}

void DeliveryManager::processAckdSequenceNumbers(const InputMemoryStream & packet)
{
	std::vector<uint32> received_acks;
	packet >> received_acks;
	for (auto seq_ack = received_acks.begin(); seq_ack != received_acks.end(); seq_ack++)
	{
		for (auto item = pending_deliveries.begin(); item != pending_deliveries.end(); item++)
		{
			if ((*item)->sequenceNumber == (*seq_ack))
			{
				//CALL ON SUCCESS;
				(*item)->to_remove = true;
			}
		}
	}

	//HOW DO I DELETE THINGS
	for (auto it = pending_deliveries.begin(); it != pending_deliveries.end(); it++)
		if ((*it)->to_remove)
		{
			pending_deliveries.erase(it);
			if (pending_deliveries.size() != 0)
			{
				it = pending_deliveries.begin();
			}
			else
				return;
		}
}

void DeliveryManager::processTimedOutPackets()
{
	for (auto item = pending_deliveries.begin(); item != pending_deliveries.end(); item++)
	{
		if ((*item)->timer.Read() > MS_TO_DELIVERY_TIMEOUT)
		{
			//TODO CALL ON FAILED
			pending_deliveries.erase(item);
		}
	}
}
