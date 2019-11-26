#include "Networks.h"


static bool isValidPlayerName(const char *name)
{
	bool containsLetters = false;
	for (; *name != '\0'; ++name)
	{
		if (*name >= 'a' && *name <= 'z') containsLetters = true;
		if (*name >= 'A' && *name <= 'Z') containsLetters = true;
	}
	return containsLetters;
}

void ScreenMainMenu::gui()
{
	ImGui::Begin("Main Menu");

	ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.45f);

	ImGui::Spacing();

	ImGui::Text("Server");

	static int localServerPort = 8888;
	ImGui::InputInt("Server port", &localServerPort);

	if (ImGui::Button("Start server"))
	{
		App->modScreen->screenGame->isServer = true;
		App->modScreen->screenGame->serverPort = localServerPort;
		App->modScreen->swapScreensWithTransition(this, App->modScreen->screenGame);
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::Text("Client");

	static char serverAddressStr[64] = "127.0.0.1";
	ImGui::InputText("Server address", serverAddressStr, sizeof(serverAddressStr));

	static int remoteServerPort = 8888;
	ImGui::InputInt("Server port", &remoteServerPort);

	static char playerNameStr[64] = "";
	ImGui::InputText("Player name", playerNameStr, sizeof(playerNameStr));

	// Role selection combo
	const char* spaceshipTypes[] = { "Shooter", "Reflector"};
	static const char* spaceshipTypeStr = spaceshipTypes[0];
	static uint8 spaceshipType = ObjectType::SHOOTER;
	if (ImGui::BeginCombo("Character##combo", spaceshipTypeStr)) // The second parameter is the label previewed before opening the combo.
	{
		for (uint8 i = 0; i < IM_ARRAYSIZE(spaceshipTypes); i++)
		{
			bool is_selected = (spaceshipTypeStr == spaceshipTypes[i]); // You can store your selection however you want, outside or inside your objects
			if (ImGui::Selectable(spaceshipTypes[i], is_selected))
			{
				spaceshipTypeStr = spaceshipTypes[i];
				switch (i) {
					case 0: {
						spaceshipType = ObjectType::SHOOTER;
						break;
					}
					case 1: {
						spaceshipType = ObjectType::REFLECTOR;
						break;
					}
				}
			}
			if (is_selected)
				ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
		}
		ImGui::EndCombo();
	}
	
	// Add team selection combo

	const char* teams[] = { "Team 1", "Team 2" };
	static const char* teamStr = teams[0];
	static uint8 team = ObjectTeam::TEAM_1;
	if (ImGui::BeginCombo("Team##combo", teamStr)) // The second parameter is the label previewed before opening the combo.
	{
		for (uint8 i = 0; i < IM_ARRAYSIZE(teams); i++) {
			bool is_selected = (teamStr == teams[i]); // You can store your selection however you want, outside or inside your objects
			if (ImGui::Selectable(teams[i], is_selected)) {
				teamStr = teams[i];
				switch (i) {
				case 0: {
					team = ObjectTeam::TEAM_1;
					break;
				}
				case 1: {
					team = ObjectTeam::TEAM_2;
					break;
				}
				}
			}
			if (is_selected)
				ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
		}
		ImGui::EndCombo();
	}

	static bool showInvalidUserName = false;

	if (ImGui::Button("Connect to server"))
	{
		if (isValidPlayerName(playerNameStr))
		{
			showInvalidUserName = false;
			App->modScreen->screenGame->isServer = false;
			App->modScreen->screenGame->serverPort = remoteServerPort;
			App->modScreen->screenGame->serverAddress = serverAddressStr;
			App->modScreen->screenGame->playerName = playerNameStr;
			App->modScreen->screenGame->spaceshipType = spaceshipType;
			App->modScreen->screenGame->team = team;
			App->modScreen->swapScreensWithTransition(this, App->modScreen->screenGame);
		}
		else
		{
			showInvalidUserName = true;
		}
	}

	if (showInvalidUserName)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.2f, 1.0f));
		ImGui::Text("Please insert a valid player name.");
		ImGui::PopStyleColor();
	}

	ImGui::PopItemWidth();

	ImGui::End();
}
