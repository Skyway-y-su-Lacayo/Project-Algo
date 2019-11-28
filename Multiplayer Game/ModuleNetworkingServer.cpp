#include "Networks.h"
#include "ModuleNetworkingServer.h"



//////////////////////////////////////////////////////////////////////
// ModuleNetworkingServer public methods
//////////////////////////////////////////////////////////////////////

void ModuleNetworkingServer::setListenPort(int port)
{
	listenPort = port;
}



//////////////////////////////////////////////////////////////////////
// ModuleNetworking virtual methods
//////////////////////////////////////////////////////////////////////

void ModuleNetworkingServer::onStart()
{
	if (!createSocket()) return;

	// Reuse address
	int enable = 1;
	int res = setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(int));
	if (res == SOCKET_ERROR) {
		reportError("ModuleNetworkingServer::start() - setsockopt");
		disconnect();
		return;
	}

	// Create and bind to local address
	if (!bindSocketToPort(listenPort)) {
		return;
	}

	state = ServerState::Listening;

	// Start timers
	sendPingTimer.Start();
	sendReplicationTimer.Start();

	// Instantiate boundaries
	// Top
	App->modGameObject->spawnBackground({ 0,550 }, { 1200, 100 });
	// Bottom
	App->modGameObject->spawnBackground({ 0,-550 }, { 1200, 100 });
	// Right
	App->modGameObject->spawnBackground({ 550,0 }, { 100, 1000 });
	// Left
	App->modGameObject->spawnBackground({ -550,0 }, { 100, 1000 });
}

void ModuleNetworkingServer::onGui()
{
	if (ImGui::CollapsingHeader("ModuleNetworkingServer", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("Connection checking info:");
		ImGui::Text(" - Ping interval (s): %f", PING_INTERVAL_SECONDS);
		ImGui::Text(" - Disconnection timeout (s): %f", DISCONNECT_TIMEOUT_SECONDS);
		ImGui::Text(" - Pings Received : %i", pingsReceived);
		ImGui::Checkbox("Block Pings", &blockPingsSend);
		ImGui::Checkbox("Disconnection by pings", &disconnectionByPings);

		ImGui::Separator();

		ImGui::Text("Replication");
		ImGui::InputFloat("Delivery interval (s)", &replicationDeliveryIntervalSeconds, 0.01f, 0.1f);
		
		ImGui::Separator();

		if (state == ServerState::Listening)
		{
			int count = 0;

			for (int i = 0; i < MAX_CLIENTS; ++i)
			{
				if (clientProxies[i].name != "")
				{
					ImGui::Text("CLIENT %d", count++);
					ImGui::Text(" - address: %d.%d.%d.%d",
						clientProxies[i].address.sin_addr.S_un.S_un_b.s_b1,
						clientProxies[i].address.sin_addr.S_un.S_un_b.s_b2,
						clientProxies[i].address.sin_addr.S_un.S_un_b.s_b3,
						clientProxies[i].address.sin_addr.S_un.S_un_b.s_b4);
					ImGui::Text(" - port: %d", ntohs(clientProxies[i].address.sin_port));
					ImGui::Text(" - name: %s", clientProxies[i].name.c_str());
					ImGui::Text(" - id: %d", clientProxies[i].clientId);
					ImGui::Text(" - Last packet time: %.04f", clientProxies[i].lastPacketReceivedTime);
					ImGui::Text(" - Seconds since repl.: %.04f", clientProxies[i].secondsSinceLastReplication);

					ImGui::Separator();
					ImGui::Text("Delivery Manager info");
					ImGui::Text("Seq number: %lu", clientProxies[i].deliveryManager.seq_number);
					ImGui::Text("Pending Deliveries SIZE : %lu numbers:",clientProxies[i].deliveryManager.pending_deliveries.size());
					for (int t = 0; t < clientProxies[i].deliveryManager.pending_deliveries.size(); t++)
					{
						ImGui::SameLine();
						ImGui::Text("%lu", clientProxies[i].deliveryManager.pending_deliveries[t]->sequenceNumber);
					}

					
					ImGui::Separator();
				}
			}

			ImGui::Checkbox("Render colliders", &App->modRender->mustRenderColliders);
		}
	}
}

void ModuleNetworkingServer::onPacketReceived(const InputMemoryStream &packet, const sockaddr_in &fromAddress)
{
	if (state == ServerState::Listening)
	{
		// Register player
		ClientProxy *proxy = getClientProxy(fromAddress);

		// Read the packet type
		ClientMessage message;
		packet >> message;

		// Process the packet depending on its type
		if (message == ClientMessage::Hello) {

			if (proxy == nullptr) {

				std::string playerName;
				uint8 spaceshipType;
				uint8 team;
				packet >> playerName;
				packet >> spaceshipType;
				packet >> team;

				// Check if the combination of spaceshipType and team is possible
				if (!checkSpaceshipAndTeam(spaceshipType, team)) {
					OutputMemoryStream unwelcomePacket;
					unwelcomePacket << ServerMessage::Unwelcome;
					sendPacket(unwelcomePacket, fromAddress);

					WLOG("Message received: UNWELCOMED hello - from player %s", playerName.c_str());
				}else{
					proxy = createClientProxy();

					proxy->address.sin_family = fromAddress.sin_family;
					proxy->address.sin_addr.S_un.S_addr = fromAddress.sin_addr.S_un.S_addr;
					proxy->address.sin_port = fromAddress.sin_port;
					proxy->connected = true;
					proxy->name = playerName;
					proxy->clientId = nextClientId++;

					// Start "receive Ping timer"
					proxy->receivePingTimer.Start();

					// Create new network object
					spawnPlayer(*proxy, spaceshipType, team);

					// Send welcome to the new player
					OutputMemoryStream welcomePacket;
					welcomePacket << ServerMessage::Welcome;
					welcomePacket << proxy->clientId;
					welcomePacket << proxy->gameObject->networkId;
					sendPacket(welcomePacket, fromAddress);

			

					uint16 networkGameObjectsCount;
					GameObject *networkGameObjects[MAX_NETWORK_OBJECTS];
					App->modLinkingContext->getNetworkGameObjects(networkGameObjects, &networkGameObjectsCount);
					for (uint16 i = 0; i < networkGameObjectsCount; ++i) {
						GameObject *gameObject = networkGameObjects[i];
						if (gameObject->networkId != proxy->gameObject->networkId)
						{
							// TODO(jesus): Notify this proxy's replication manager about the creation of this game object
							proxy->replicationManager.create(gameObject->networkId);
							proxy->replicationManager.update(gameObject->networkId);
						}
						// TODO(jesus): Notify the new client proxy's replication manager about the creation of this game object
					}

					LOG("Message received: hello - from player %s", playerName.c_str());
				}
			}
			else {
				OutputMemoryStream unwelcomePacket;
				unwelcomePacket << ServerMessage::Unwelcome;
				sendPacket(unwelcomePacket, fromAddress);

				WLOG("Message received: UNWELCOMED hello - from player %s", proxy->name.c_str());
			}
		}
		else if (message == ClientMessage::Input && proxy != nullptr) {
			// Process the input packet and update the corresponding game object
			if (proxy != nullptr) {
				// Read input data
				while (packet.RemainingByteCount() > 0) {
					InputPacketData inputData;
					packet >> inputData.sequenceNumber;
					packet >> inputData.horizontalAxis;
					packet >> inputData.verticalAxis;
					packet >> inputData.buttonBits;
					packet >> inputData.mouse_x;
					packet >> inputData.mouse_y;
					packet >> inputData.mouseState;

					if (inputData.sequenceNumber >= proxy->nextExpectedInputSequenceNumber) {
						// Buttons
						proxy->gamepad.horizontalAxis = inputData.horizontalAxis;
						proxy->gamepad.verticalAxis = inputData.verticalAxis;
						unpackInputControllerButtons(inputData.buttonBits, proxy->gamepad);

						// Mouse
						// Change mouse input coordinate origin


						vec2 winSize = App->modRender->getWindowsSize();
						proxy->mouse.x = inputData.mouse_x - winSize.x / 2;
						proxy->mouse.y = inputData.mouse_y - winSize.y / 2;
						proxy->mouse.buttons[0] = inputData.mouseState;


					
						
						// Send imput to gameObject to process
						proxy->gameObject->behaviour->onInput(proxy->gamepad, proxy->mouse);
						proxy->nextExpectedInputSequenceNumber = inputData.sequenceNumber + 1;
					}
				}
			}
		}
		else if (message == ClientMessage::Ping && proxy != nullptr) {
		proxy->deliveryManager.processAckdSequenceNumbers(packet);
			proxy->receivePingTimer.Start();
			pingsReceived++;
		}
	

		if (proxy != nullptr)
		{
			proxy->lastPacketReceivedTime = Time.time;
		}
	}
}

void ModuleNetworkingServer::onUpdate()
{
	if (state == ServerState::Listening)
	{
		// Update clients

		for (ClientProxy &clientProxy : clientProxies)
		{
			if (clientProxy.connected)
			{
				// Disconnect client if it hasen't send ping for a while
				manageReceivePing(&clientProxy);

				OutputMemoryStream packet;
				packet << ServerMessage::Replication;

				// TODO(jesus): If the replication interval passed and the replication manager of this proxy
				//              has pending data, write and send a replication packet to this client.
			}
		}

		for (int i = 0; i < MAX_CLIENTS; ++i)
		{
			if (clientProxies[i].connected)
			{
				clientProxies[i].deliveryManager.processTimedOutPackets();
			}
		}
		// Send ping to all clients periodically
		manageSendPing();

		// Send replication to all clients periodically
		manageSendReplication();
	}
}

void ModuleNetworkingServer::onConnectionReset(const sockaddr_in & fromAddress)
{
	// Find the client proxy
	ClientProxy *proxy = getClientProxy(fromAddress);

	if (proxy)
	{
		// Notify game object deletion to replication managers
		for (int i = 0; i < MAX_CLIENTS; ++i)
		{
			if (clientProxies[i].connected && proxy->clientId != clientProxies[i].clientId)
			{
				// TODO(jesus): Notify this proxy's replication manager about the destruction of this player's game object
				clientProxies[i].replicationManager.destroy(proxy->gameObject->networkId);
			}
		}
		// Clear the client proxy
		destroyClientProxy(proxy);
	}
}

void ModuleNetworkingServer::onDisconnect()
{
	// Destroy network game objects
	uint16 netGameObjectsCount;
	GameObject *netGameObjects[MAX_NETWORK_OBJECTS];
	App->modLinkingContext->getNetworkGameObjects(netGameObjects, &netGameObjectsCount);
	for (uint32 i = 0; i < netGameObjectsCount; ++i)
	{
		NetworkDestroy(netGameObjects[i]);
	}

	// Clear all client proxies
	for (ClientProxy &clientProxy : clientProxies)
	{
		clientProxy = {};
	}
	
	nextClientId = 0;

	state = ServerState::Stopped;
}





//////////////////////////////////////////////////////////////////////
// Client proxies
//////////////////////////////////////////////////////////////////////

ModuleNetworkingServer::ClientProxy * ModuleNetworkingServer::getClientProxy(const sockaddr_in &clientAddress)
{
	// Try to find the client
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].address.sin_addr.S_un.S_addr == clientAddress.sin_addr.S_un.S_addr &&
			clientProxies[i].address.sin_port == clientAddress.sin_port)
		{
			return &clientProxies[i];
		}
	}

	return nullptr;
}

ModuleNetworkingServer::ClientProxy * ModuleNetworkingServer::createClientProxy()
{
	// If it does not exist, pick an empty entry
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (!clientProxies[i].connected)
		{
			return &clientProxies[i];
		}
	}

	return nullptr;
}

void ModuleNetworkingServer::destroyClientProxy(ClientProxy * proxy)
{
	destroyNetworkObject(proxy->gameObject);
	*proxy = {};
}

void ModuleNetworkingServer::destroyClientProxyByGO(GameObject * spaceship) {

	for (auto & clientProxy : clientProxies){
		if (clientProxy.gameObject == spaceship){
			destroyClientProxy(&clientProxy);
			break;
		}
	}
}

void ModuleNetworkingServer::sendPacketAll(OutputMemoryStream& data) {

	for (ClientProxy &clientProxy : clientProxies) {
		if(clientProxy.connected)
			sendPacket(data, clientProxy.address);
	}
}

uint32 ModuleNetworkingServer::connectedClients()
{
	uint32 ret = 0;
	for (ClientProxy &clientProxy : clientProxies) {
		if (clientProxy.connected)
			ret++;
	}
	return ret;
}

//////////////////////////////////////////////////////////////////////
// Spawning
//////////////////////////////////////////////////////////////////////


GameObject * ModuleNetworkingServer::spawnPlayer(ClientProxy & clientProxy, uint8 type, uint8 team) {
	GameObject* ret = nullptr;

	switch ((ObjectType)type) {
		case ObjectType::SHOOTER:
		{
			ret = spawnPlayerShooter(clientProxy, team);
			break;
		}
		case ObjectType::REFLECTOR:
		{
			ret = spawnPlayerReflector(clientProxy, team);
			break;
		}
	}

	return ret;
}

GameObject * ModuleNetworkingServer::spawnPlayerShooter(ClientProxy &clientProxy, uint8 team)
{
	// Create a new game object with the player properties
	clientProxy.gameObject = Instantiate();
	clientProxy.gameObject->size = { 100, 100 };
	// Scale
	clientProxy.gameObject->size *= App->modGameObject->gameScale;


	// Initial position
	clientProxy.gameObject->position.y = -300;
	team == ObjectTeam::TEAM_1 ? clientProxy.gameObject->position.x = -300 : clientProxy.gameObject->position.x = 300;
	clientProxy.gameObject->angle = 45.0f;

	// Shooter texture
	if(team == ObjectTeam::TEAM_1)
		clientProxy.gameObject->texture = App->modResources->T1_Shooter;
	else
		clientProxy.gameObject->texture = App->modResources->T2_Shooter;

	// Create collider
	clientProxy.gameObject->collider = App->modCollision->addCollider(ColliderType::Player, clientProxy.gameObject);
	clientProxy.gameObject->collider->isTrigger = true;

	// Create behaviour
	clientProxy.gameObject->behaviour = new Shooter;
	clientProxy.gameObject->behaviour->gameObject = clientProxy.gameObject;

	// Assign tag
	clientProxy.gameObject->tag = ObjectType::SHOOTER;

	//Assign Team
	clientProxy.gameObject->team = team;

	// Assign a new network identity to the object
	App->modLinkingContext->registerNetworkGameObject(clientProxy.gameObject);

	// Notify all client proxies' replication manager to create the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			// TODO(jesus): Notify this proxy's replication manager about the creation of this game object
			clientProxies[i].replicationManager.create(clientProxy.gameObject->networkId);
		}
	}

	return clientProxy.gameObject;
}

GameObject * ModuleNetworkingServer::spawnPlayerReflector(ClientProxy & clientProxy, uint8 team) {

	// Create a new game object with the player properties
	clientProxy.gameObject = Instantiate();
	clientProxy.gameObject->size = { 100, 100 };
	// Scale
	clientProxy.gameObject->size *= App->modGameObject->gameScale;
	
	// Initial position
	clientProxy.gameObject->position.y = 300;
	team == ObjectTeam::TEAM_1 ? clientProxy.gameObject->position.x = -300 : clientProxy.gameObject->position.x = 300;
	clientProxy.gameObject->angle = 45.0f;

	// Reflector texture
	if(team == ObjectTeam::TEAM_1)
		clientProxy.gameObject->texture = App->modResources->T1_Reflector;
	else 
		clientProxy.gameObject->texture = App->modResources->T2_Reflector;


	// Create collider
	clientProxy.gameObject->collider = App->modCollision->addCollider(ColliderType::Player, clientProxy.gameObject);
	clientProxy.gameObject->collider->isTrigger = true;

	clientProxy.gameObject->behaviour = new Reflector;
	clientProxy.gameObject->behaviour->gameObject = clientProxy.gameObject;

	// Assign tag
	clientProxy.gameObject->tag = ObjectType::REFLECTOR;

	// Assign team (Hardcoded to team 1 for testing purposes
	clientProxy.gameObject->team = team;

	// Assign a new network identity to the object
	App->modLinkingContext->registerNetworkGameObject(clientProxy.gameObject);

	// Notify all client proxies' replication manager to create the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (clientProxies[i].connected) {
			// TODO(jesus): Notify this proxy's replication manager about the creation of this game object
			clientProxies[i].replicationManager.create(clientProxy.gameObject->networkId);
		}
	}
	 
	return clientProxy.gameObject;
}

GameObject * ModuleNetworkingServer::spawnReflectorBarrier(GameObject* parent) {
	GameObject* reflector_barrier = Instantiate();
	reflector_barrier->size = { 100, 50 };
	// Scale
	reflector_barrier->size *= App->modGameObject->gameScale;
	reflector_barrier->angle = 0;

	// Texture 
	if(parent->team == ObjectTeam::TEAM_1)
		reflector_barrier->texture = App->modResources->T1_ReflectorBarrier;
	else
		reflector_barrier->texture = App->modResources->T2_ReflectorBarrier;

	// Create collider
	reflector_barrier->collider = App->modCollision->addCollider(ColliderType::Player, reflector_barrier);
	reflector_barrier->collider->isTrigger = true;

	// Behaviour (Empty for now, since we just want to test visually)
	reflector_barrier->behaviour = new ReflectorBarrier;
	reflector_barrier->behaviour->gameObject = reflector_barrier;

	// Tag and team
	reflector_barrier->tag = ObjectType::REFLECTOR_BARRIER;
	reflector_barrier->team = parent->team;

	// Register go
	App->modLinkingContext->registerNetworkGameObject(reflector_barrier);

	// Notify all client proxies' replication manager to create the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (clientProxies[i].connected) {
			// TODO(jesus): Notify this proxy's replication manager about the creation of this game object
			clientProxies[i].replicationManager.create(reflector_barrier->networkId);
		}
	}

	return reflector_barrier;
}

GameObject * ModuleNetworkingServer::spawnBullet(GameObject *parent, ColliderType col_type)
{
	// Create a new game object with the player properties
	GameObject *gameObject = Instantiate();

	gameObject->angle = parent->angle;
	gameObject->position = parent->position;

	// Tag and texture
	switch (col_type) {
		case ColliderType::SoftLaser: {
			if (parent->team == ObjectTeam::TEAM_1)
				gameObject->texture = App->modResources->T1_SoftProjectile;
			else
				gameObject->texture = App->modResources->T2_SoftProjectile;

			gameObject->tag = ObjectType::SOFT_LASER;
			gameObject->size = { 20, 60 };
			break;
		}
		case ColliderType::HardLaser: {
			if (parent->team == ObjectTeam::TEAM_1)
				gameObject->texture = App->modResources->T1_HardProjectile;
			else
				gameObject->texture = App->modResources->T2_HardProjectile;

			gameObject->tag = ObjectType::HARD_LASER;
			gameObject->size = { 60, 60 };
		}
									 
	}

	// Scale
	gameObject->size *= App->modGameObject->gameScale;

	// Create collider
	gameObject->collider = App->modCollision->addCollider(col_type, gameObject);

	// Create behaviour
	gameObject->behaviour = new Laser;
	gameObject->behaviour->gameObject = gameObject;



	gameObject->team = parent->team;
	// Assign a new network identity to the object
	App->modLinkingContext->registerNetworkGameObject(gameObject);

	// Notify all client proxies' replication manager to create the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			// TODO(jesus): Notify this proxy's replication manager about the creation of this game object
			clientProxies[i].replicationManager.create(gameObject->networkId);
		}
	}

	return gameObject;
}

bool ModuleNetworkingServer::checkSpaceshipAndTeam(uint8 type, uint8 team) {

	bool ret = false;

	int team1Members = 0;
	int team2Members = 0;
	bool team1Shooter = false;
	bool team2Shooter = false;


	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (clientProxies[i].connected) {
			if (clientProxies[i].gameObject->team == ObjectTeam::TEAM_1) {
				team1Members++;
				if (clientProxies[i].gameObject->tag == ObjectType::SHOOTER)
					team1Shooter = true;
			}
			else if(clientProxies[i].gameObject->team == ObjectTeam::TEAM_2){
				team2Members++;
				if(clientProxies[i].gameObject->tag == ObjectType::SHOOTER)  // Team 2
					team2Shooter = true;
			}
		}
	}


	if (team == ObjectTeam::TEAM_1 && team1Members < 2) {

		if (team1Members == 0)
			ret = true;
		else if (!team1Shooter && type == ObjectType::SHOOTER) // There is 1 reflector and player wants to be a shooter
			ret = true;
		else if (team1Shooter && type == ObjectType::REFLECTOR) // There is 1 shooter and player wants to be a reflector
			ret = true;

	}
	else if(team == ObjectTeam::TEAM_2 && team2Members < 2) { // Team 2
		if (team2Members == 0)
			ret = true;
		else if (!team2Shooter && type == ObjectType::SHOOTER) // There is 1 reflector and player wants to be a shooter
			ret = true;
		else if (team2Shooter && type == ObjectType::REFLECTOR) // There is 1 shooter and player wants to be a reflector
			ret = true;
	}


	return ret;
}


//////////////////////////////////////////////////////////////////////
// Update / destruction
//////////////////////////////////////////////////////////////////////

void ModuleNetworkingServer::destroyNetworkObject(GameObject * gameObject)
{
	// Notify all client proxies' replication manager to destroy the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			// TODO(jesus): Notify this proxy's replication manager about the destruction of this game object
			clientProxies[i].replicationManager.destroy(gameObject->networkId);
		}
	}

	// Assuming the message was received, unregister the network identity
	App->modLinkingContext->unregisterNetworkGameObject(gameObject);

	// Finally, destroy the object from the server
	Destroy(gameObject);
}

void ModuleNetworkingServer::updateNetworkObject(GameObject * gameObject)
{
	// Notify all client proxies' replication manager to destroy the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			// TODO(jesus): Notify this proxy's replication manager about the update of this game object
			clientProxies[i].replicationManager.update(gameObject->networkId);
		}
	}
}

void ModuleNetworkingServer::manageSendReplication() {
	if (sendReplicationTimer.ReadSeconds() > replicationDeliveryIntervalSeconds) {
		for (int i = 0; i < MAX_CLIENTS; ++i) {
			if (clientProxies[i].connected) {
				OutputMemoryStream packet;
				packet << ServerMessage::Replication;
				Delivery* delivery = clientProxies[i].deliveryManager.writeSequenceNumber(packet);
				//TODO find a better way to do this
				delivery->delegate = new ReplicationDelegate(this, clientProxies[i].replicationManager.actions);

				clientProxies[i].replicationManager.write(packet,delivery);

				sendPacket(packet, clientProxies[i].address);
			}
		}

		sendReplicationTimer.Start();
	}

}

void ModuleNetworkingServer::manageReceivePing(ClientProxy * clientProxy) {
	if (clientProxy->receivePingTimer.ReadSeconds() > DISCONNECT_TIMEOUT_SECONDS && disconnectionByPings) 
		destroyClientProxy(clientProxy);
	
		
}

void ModuleNetworkingServer::manageSendPing() {
	if (sendPingTimer.ReadSeconds() > PING_INTERVAL_SECONDS && !blockPingsSend) {
		OutputMemoryStream out;
		out << ServerMessage::Ping;
		sendPacketAll(out);
		sendPingTimer.Start();
	}
}


//////////////////////////////////////////////////////////////////////
// Global update / destruction of game objects
//////////////////////////////////////////////////////////////////////

void NetworkUpdate(GameObject * gameObject)
{
	ASSERT(App->modNetServer->isConnected());

	App->modNetServer->updateNetworkObject(gameObject);
}

void NetworkDestroy(GameObject * gameObject)
{
	ASSERT(App->modNetServer->isConnected());

	App->modNetServer->destroyNetworkObject(gameObject);
}
