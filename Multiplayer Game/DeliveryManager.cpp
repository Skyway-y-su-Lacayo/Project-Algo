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
	packet << seq_number;
	seq_number++;

	return nullptr;
}
