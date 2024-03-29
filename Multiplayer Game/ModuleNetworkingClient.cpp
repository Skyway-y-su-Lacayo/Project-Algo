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

	// Spawn "Waiting For Players" Label
	waitingLabel = Instantiate();

	waitingLabel->position = { 0,0 };
	waitingLabel->texture = App->modResources->waiting;
	waitingLabel->order = 10;
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
			float playerAngle = 0;
			GameObject *playerGameObject = App->modLinkingContext->getNetworkGameObject(networkId);
			if (playerGameObject != nullptr) {
				playerPosition = playerGameObject->position;
				playerAngle = playerGameObject->angle;
			}
			ImGui::Text(" - Coordinates: (%f, %f)", playerPosition.x, playerPosition.y);
			ImGui::Text(" - Angle: %f", playerAngle);

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

			ImGui::Checkbox("Clientside Prediction", &clientside_prediction);

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
					packet >> last_server_frame;
					replicationClient.read(packet);
				}
			}
		}
	
}

void ModuleNetworkingClient::onUpdate()
{
	// Independing from the client state

	if (waitingForPlayers && App->modGameObject->playersReady()) {
		waitingForPlayers = false;
		waitingLabel->state = GameObject::State::NON_EXISTING;
	}


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


		// Can't play while waiting for other players, input is nulated
		if (waitingForPlayers) return;


		// Interpolation: Pass the client's networkID which WON't be interpolated. We use "client side prediction" (Lorien :)
		App->modGameObject->calculateInterpolation(networkId);

		secondsSinceLastInputDelivery += Time.deltaTime;

		InputPacketData inputPacketData;
		inputPacketData.inputFrame = current_frame;
		inputPacketData.horizontalAxis = Input.horizontalAxis;
		inputPacketData.verticalAxis = Input.verticalAxis;
		inputPacketData.buttonBits = packInputControllerButtons(Input);

		inputPacketData.mouse_x = Mouse.x;
		inputPacketData.mouse_y = Mouse.y;
		inputPacketData.mouseState = Mouse.buttons[0]; //Hopefully, right button

		previous_inputs.push_back(inputPacketData);

		if (inputDataBack - inputDataFront < ArrayCount(inputData))
		{
			uint32 currentInputData = inputDataBack++;
			InputPacketData &inputPacketData = inputData[currentInputData % ArrayCount(inputData)];
			inputPacketData.inputFrame = current_frame;
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
					packet << current_frame;
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
		current_frame += 1;
	}

	// Make the camera focus the player game object
	// ClientSidePrediction
	GameObject *playerGO = App->modLinkingContext->getNetworkGameObject(networkId);
	if (playerGO != nullptr)
	{
		if (clientside_prediction)
		{
			if (!playerGO->new_packet)
			{
				MouseController mouse;
				vec2 winSize = App->modRender->getWindowsSize();
				mouse.x = Mouse.x - winSize.x / 2;
				mouse.y = Mouse.y - winSize.y / 2;
				playerGO->behaviour->onInput(Input, mouse);
			}
			else
				clientSidePrediction(playerGO);
		}

		else {
			playerGO->position = playerGO->final_pos;
			playerGO->angle = playerGO->final_angle;
		}

		App->modRender->cameraPosition = playerGO->position;
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

	// Erase UI
	Destroy(waitingLabel);

	teamTagsCount = 0;
	for (int i = 0; i < 4; i++)
		if (teamTags[i]) 
			Destroy(teamTags[i]);

	// Reset vars
	waitingForPlayers = true;

	deliveryManager.clear();
	last_server_frame = 0;
	App->modRender->cameraPosition = {};
}

void ModuleNetworkingClient::clientSidePrediction(GameObject* go)
{
	go->position = go->final_pos;
	go->angle = go->final_angle;
	go->new_packet = false;

	for (auto input : previous_inputs)
	{
		if (input.inputFrame > last_server_frame)
		{
			InputController gamepad;
			MouseController mouse;
			gamepad.horizontalAxis = input.horizontalAxis;
			gamepad.verticalAxis = input.verticalAxis;
			unpackInputControllerButtons(input.buttonBits,gamepad);

			// Mouse
			// Change mouse input coordinate origin
			vec2 winSize = App->modRender->getWindowsSize();
			mouse.x = input.mouse_x - winSize.x / 2;
			mouse.y = input.mouse_y - winSize.y / 2;
			mouse.buttons[0] = input.mouseState;

			// Send imput to gameObject to process
			go->behaviour->onInput(gamepad, mouse);
		}
	}
	previous_inputs.clear();
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

GameObject* ModuleNetworkingClient::spawnPlayer(uint32 networkID, uint8 tag, uint8 team) {

	GameObject* gameObject = Instantiate();
	gameObject->size = { 100, 100 };
	gameObject->angle = 45.0f;
	gameObject->team = team;

	teamTags[teamTagsCount++] = spawnTeamTag(gameObject);

	switch (tag) {
		case ObjectType::SHOOTER: {
			if (networkID == networkId)
				gameObject->behaviour = new ShooterClient(gameObject);
			else {
				gameObject->behaviour = new Player;
				gameObject->behaviour->gameObject = gameObject;
			}


			if(team == ObjectTeam::TEAM_1)
				gameObject->texture = App->modResources->T1_Shooter;
			else {
				gameObject->texture = App->modResources->T2_Shooter;
				gameObject->size = { 130, 130 };
			}
			break;
		}
		case ObjectType::REFLECTOR: {
			if (networkID == networkId)
				gameObject->behaviour = new ReflectorClient(gameObject);
			else
			{
				gameObject->behaviour = new Player;
				gameObject->behaviour->gameObject = gameObject;
			}

			if (team == ObjectTeam::TEAM_1)
				gameObject->texture = App->modResources->T1_Reflector;
			else 
				gameObject->texture = App->modResources->T2_Reflector;

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

GameObject * ModuleNetworkingClient::spawnTeamTag(GameObject * player) {

	GameObject* gameObject = Instantiate();
	gameObject->size = { 164, 34 };
	gameObject->angle = 0.0f;


	gameObject->behaviour = new PlayerHealth();
	gameObject->behaviour->gameObject = gameObject;
	((PlayerHealth*)gameObject->behaviour)->player = player;

	
	player->team == ObjectTeam::TEAM_1 ? gameObject->texture = App->modResources->T1_Hp : gameObject->texture = App->modResources->T2_Hp;

	// Scale
	gameObject->size *= App->modGameObject->gameScale;

	// Assign tag
	gameObject->tag = ObjectType::UI;
	// Ui allways blits last
	gameObject->order = 10;

	return gameObject;
}

GameObject* ModuleNetworkingClient::spawnReflectorBarrier(uint32 networkID, uint8 team) {

	GameObject* gameObject = Instantiate();
	gameObject->size = { 100, 50 };
	// Scale
	gameObject->size *= App->modGameObject->gameScale;
	gameObject->angle = 45.0f;

	if (team == ObjectTeam::TEAM_1)
		gameObject->texture = App->modResources->T1_ReflectorBarrier;
	else 
		gameObject->texture = App->modResources->T2_ReflectorBarrier;

	gameObject->team = team;
	// No collider needed

	// No behaviour needed

	// Assign tag
	gameObject->tag = ObjectType::REFLECTOR_BARRIER;

	// Assign a new network identity to the object
	App->modLinkingContext->registerNetworkGameObjectWithNetworkId(gameObject, networkID);

	//Find the Reflector for the barrier
	if (GameObject* playerGO = App->modLinkingContext->getNetworkGameObject(networkId)) {
		if (gameObject->team == playerGO->team && playerGO->tag == ObjectType::REFLECTOR)
		{
			ReflectorClient* behaviour = (ReflectorClient*)playerGO->behaviour;
			if (!behaviour->reflector_barrier)
				behaviour->reflector_barrier = gameObject;
		}
	}
	return gameObject;

}

GameObject* ModuleNetworkingClient::spawnBullet(uint32 networkID, uint8 tag, uint8 team) {

	GameObject* gameObject = Instantiate();

	gameObject->angle = 45.0f;

	switch (tag) {
		case ObjectType::SOFT_LASER: {

			if (team == ObjectTeam::TEAM_1)
				gameObject->texture = App->modResources->T1_SoftProjectile;
			else
				gameObject->texture = App->modResources->T2_SoftProjectile;

			gameObject->size = { 20, 60 };
			break;
		}
		case ObjectType::HARD_LASER: {

			if (team == ObjectTeam::TEAM_1)
				gameObject->texture = App->modResources->T1_HardProjectile;
			else
				gameObject->texture = App->modResources->T2_HardProjectile;
			gameObject->size = { 60, 60 };
			break;
		}
	}
	// Scale
	gameObject->size *= App->modGameObject->gameScale;
	
	// Assign tag
	gameObject->tag = tag;

	// Assign behaviour
	gameObject->behaviour = new ClientLaser;
	gameObject->behaviour->gameObject = gameObject;

	//Assign order
	gameObject->order = -1;
	// Assign a new network identity to the object
	App->modLinkingContext->registerNetworkGameObjectWithNetworkId(gameObject, networkID);

	return gameObject;
}