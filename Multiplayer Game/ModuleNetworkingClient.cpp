#include "Networks.h"
#include "ModuleNetworkingClient.h"



//////////////////////////////////////////////////////////////////////
// ModuleNetworkingClient public methods
//////////////////////////////////////////////////////////////////////


void ModuleNetworkingClient::setServerAddress(const char * pServerAddress, uint16 pServerPort)
{
	serverAddressStr = pServerAddress;
	serverPort = pServerPort;
}

void ModuleNetworkingClient::setPlayerInfo(const char * pPlayerName, uint8 pSpaceshipType, uint8 pTeam)
{
	playerName = pPlayerName;
	spaceshipType = pSpaceshipType;
	team = pTeam;
}



//////////////////////////////////////////////////////////////////////
// ModuleNetworking virtual methods
//////////////////////////////////////////////////////////////////////

void ModuleNetworkingClient::onStart()
{
	if (!createSocket()) return;

	if (!bindSocketToPort(0)) {
		disconnect();
		return;
	}

	// Create remote address
	serverAddress = {};
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(serverPort);
	int res = inet_pton(AF_INET, serverAddressStr.c_str(), &serverAddress.sin_addr);
	if (res == SOCKET_ERROR) {
		reportError("ModuleNetworkingClient::startClient() - inet_pton");
		disconnect();
		return;
	}

	state = ClientState::Start;

	inputDataFront = 0;
	inputDataBack = 0;

	secondsSinceLastInputDelivery = 0.0f;
	lastPacketReceivedTime = Time.time;

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

void ModuleNetworkingClient::onGui()
{
	if (state == ClientState::Stopped) return;

	if (ImGui::CollapsingHeader("ModuleNetworkingClient", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (state == ClientState::WaitingWelcome)
		{
			ImGui::Text("Waiting for server response...");
		}
		else if (state == ClientState::Playing)
		{
			ImGui::Text("Connected to server");

			ImGui::Separator();

			ImGui::Text("Player info:");
			ImGui::Text(" - Id: %u", playerId);
			ImGui::Text(" - Name: %s", playerName.c_str());

			ImGui::Separator();

			ImGui::Text("Spaceship info:");
			ImGui::Text(" - Type: %u", spaceshipType);
			ImGui::Text(" - Network id: %u", networkId);

			vec2 playerPosition = {};
			GameObject *playerGameObject = App->modLinkingContext->getNetworkGameObject(networkId);
			if (playerGameObject != nullptr) {
				playerPosition = playerGameObject->position;
			}
			ImGui::Text(" - Coordinates: (%f, %f)", playerPosition.x, playerPosition.y);

			ImGui::Separator();

			ImGui::Text("Connection checking info:");
			ImGui::Text(" - Ping interval (s): %f", PING_INTERVAL_SECONDS);
			ImGui::Text(" - Disconnection timeout (s): %f", DISCONNECT_TIMEOUT_SECONDS);
			ImGui::Text(" - Pings Received : %i", pingsReceived);
			ImGui::Checkbox("Block Pings", &blockPingsSend);
			ImGui::Checkbox("Disconnection by pings", &disconnectionByPings);

			ImGui::Separator();
			ImGui::Text("Delivery Manager info");
			ImGui::Text("Seq number: %lu", deliveryManager.seq_number);
			ImGui::Text("Pending Seq numbers:");
			for (int i = 0; i < deliveryManager.pending_ack.size(); i++)
			{
				ImGui::SameLine();
				ImGui::Text("%lu", deliveryManager.pending_ack[i]);
			}

			ImGui::Separator();

			ImGui::Text("Input:");
			ImGui::InputFloat("Delivery interval (s)", &inputDeliveryIntervalSeconds, 0.01f, 0.1f, 4);
		}
	}
}

void ModuleNetworkingClient::onPacketReceived(const InputMemoryStream &packet, const sockaddr_in &fromAddress)
{
	lastPacketReceivedTime = Time.time;

	ServerMessage message;
	packet >> message;

		if (state == ClientState::WaitingWelcome)
		{
			if (message == ServerMessage::Welcome)
			{
				packet >> playerId;
				packet >> networkId;

				LOG("ModuleNetworkingClient::onPacketReceived() - Welcome from server");
				state = ClientState::Playing;
			}
			else if (message == ServerMessage::Unwelcome)
			{
				WLOG("ModuleNetworkingClient::onPacketReceived() - Unwelcome from server :-(");
				disconnect();
			}
		}
		else if (state == ClientState::Playing)
		{
			// TODO(jesus): Handle incoming messages from server
			if (message == ServerMessage::Ping) {
				receivePingTimer.Start();
				pingsReceived++;
			}
			if (message == ServerMessage::Replication) {
				if (deliveryManager.processSequenceNumber(packet))
				{
					replicationClient.read(packet);
				}
			}
			if (message == ServerMessage::RepeatReplication) {

				replicationClient.read(packet);

			}
		}
	
}

void ModuleNetworkingClient::onUpdate()
{
	if (state == ClientState::Stopped) return;

	if (state == ClientState::Start)
	{
		// Send the hello packet with player data

		OutputMemoryStream stream;
		stream << ClientMessage::Hello;
		stream << playerName;
		stream << spaceshipType;
		stream << team;

		sendPacket(stream, serverAddress);

		state = ClientState::WaitingWelcome;

		initializePing();

	}
	else if (state == ClientState::WaitingWelcome)
	{
	}
	else if (state == ClientState::Playing)
	{
		// Check last ping and disconnect
		managePing(serverAddress);


		secondsSinceLastInputDelivery += Time.deltaTime;

		if (inputDataBack - inputDataFront < ArrayCount(inputData))
		{
			uint32 currentInputData = inputDataBack++;
			InputPacketData &inputPacketData = inputData[currentInputData % ArrayCount(inputData)];
			inputPacketData.sequenceNumber = currentInputData;
			inputPacketData.horizontalAxis = Input.horizontalAxis;
			inputPacketData.verticalAxis = Input.verticalAxis;
			inputPacketData.buttonBits = packInputControllerButtons(Input);
			
			inputPacketData.mouse_x = Mouse.x;
			inputPacketData.mouse_y = Mouse.y;
			inputPacketData.mouseState = Mouse.buttons[0]; //Hopefully, right button

			// Create packet (if there's input and the input delivery interval exceeded)
			if (secondsSinceLastInputDelivery > inputDeliveryIntervalSeconds)
			{
				secondsSinceLastInputDelivery = 0.0f;

				OutputMemoryStream packet;
				packet << ClientMessage::Input;
				for (uint32 i = inputDataFront; i < inputDataBack; ++i)
				{
					InputPacketData &inputPacketData = inputData[i % ArrayCount(inputData)];
					packet << inputPacketData.sequenceNumber;
					packet << inputPacketData.horizontalAxis;
					packet << inputPacketData.verticalAxis;
					packet << inputPacketData.buttonBits;
					packet << inputPacketData.mouse_x;
					packet << inputPacketData.mouse_y;
					packet << inputPacketData.mouseState;

				}

				// Clear the queue
				inputDataFront = inputDataBack;

				sendPacket(packet, serverAddress);
			}
		}
	}

	// Make the camera focus the player game object
	GameObject *playerGameObject = App->modLinkingContext->getNetworkGameObject(networkId);
	if (playerGameObject != nullptr)
	{
		App->modRender->cameraPosition = playerGameObject->position;
	}
}

void ModuleNetworkingClient::onConnectionReset(const sockaddr_in & fromAddress)
{
	disconnect();
}

void ModuleNetworkingClient::onDisconnect()
{
	state = ClientState::Stopped;

	// Get all network objects and clear the linking context
	uint16 networkGameObjectsCount;
	GameObject *networkGameObjects[MAX_NETWORK_OBJECTS] = {};
	App->modLinkingContext->getNetworkGameObjects(networkGameObjects, &networkGameObjectsCount);
	App->modLinkingContext->clear();

	// Destroy all network objects
	for (uint32 i = 0; i < networkGameObjectsCount; ++i)
	{
		Destroy(networkGameObjects[i]);
	}

	App->modRender->cameraPosition = {};
}


void ModuleNetworkingClient::initializePing() {
	receivePingTimer.Start();
	sendPingTimer.Start();
}

void ModuleNetworkingClient::managePing(sockaddr_in otherAddress) {
	if (receivePingTimer.ReadSeconds() > DISCONNECT_TIMEOUT_SECONDS && disconnectionByPings)
		disconnect();

	if (sendPingTimer.ReadSeconds() > PING_INTERVAL_SECONDS && !blockPingsSend) {
		OutputMemoryStream out;
		out << ClientMessage::Ping; 
		deliveryManager.writeSequenceNumbersPendingAck(out);
		sendPacket(out, otherAddress);
		sendPingTimer.Start();
	}

}

GameObject* ModuleNetworkingClient::spawnPlayer(uint32 networkID, uint8 tag) {

	GameObject* gameObject = Instantiate();
	gameObject->size = { 100, 100 };
	gameObject->angle = 45.0f;


	switch (tag) {
		case ObjectType::SHOOTER: {
			gameObject->texture = App->modResources->spacecraft1;
			break;
		}
		case ObjectType::REFLECTOR: {
			gameObject->texture = App->modResources->spacecraft2;
			break;
		}
	}

	// Scale
	gameObject->size *= App->modGameObject->gameScale;

	// Assign tag
	gameObject->tag = tag;

	// Assign a new network identity to the object
	App->modLinkingContext->registerNetworkGameObjectWithNetworkId(gameObject, networkID);

	return gameObject;
}

GameObject* ModuleNetworkingClient::spawnReflector(uint32 networkID) {

	GameObject* gameObject = Instantiate();
	gameObject->size = { 100, 50 };
	// Scale
	gameObject->size *= App->modGameObject->gameScale;
	gameObject->angle = 45.0f;


	gameObject->texture = App->modResources->reflector;
	// No collider needed

	// No behaviour needed

	// Assign tag
	gameObject->tag = ObjectType::REFLECTOR_BARRIER;

	// Assign a new network identity to the object
	App->modLinkingContext->registerNetworkGameObjectWithNetworkId(gameObject, networkID);

	return gameObject;

}

GameObject* ModuleNetworkingClient::spawnBullet(uint32 networkID, uint8 tag) {

	GameObject* gameObject = Instantiate();

	gameObject->angle = 45.0f;

	switch (tag) {
		case ObjectType::SOFT_LASER: {
			gameObject->texture = App->modResources->laser;
			gameObject->size = { 20, 60 };
			break;
		}
		case ObjectType::HARD_LASER: {
			gameObject->texture = App->modResources->asteroid1;
			gameObject->size = { 60, 60 };
			break;
		}
	}
	// Scale
	gameObject->size *= App->modGameObject->gameScale;
	
	// Assign tag
	gameObject->tag = tag;

	// Assign a new network identity to the object
	App->modLinkingContext->registerNetworkGameObjectWithNetworkId(gameObject, networkID);

	return gameObject;
}