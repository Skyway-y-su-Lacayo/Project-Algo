#pragma once

enum ObjectType {
	EMPTY,
	SOFT_LASER,
	SHOOTER, // NEEDS TO BE '2'
	REFLECTOR, // NEEDS TOBE '3'
	REFLECTOR_BARRIER,
	HARD_LASER

};
enum ObjectTeam {
	NO_TEAM,
	TEAM_1,
	TEAM_2
};
struct GameObject
{
	// Interpolation (logic in Networking Client Module) - Value initialization and timer reset (Client Replication Manager)
	vec2 initial_pos;
	float initial_angle;

	vec2 final_pos;
	float final_angle;

	Timer timeSinceLastUpdate; // Calculate interpolation percentage "ModuleNetworkingServer::replicationDeliveryIntervalSeconds"

	void interpolationCreate() {
		initial_pos = final_pos = position;
		initial_angle = final_angle = angle;

		timeSinceLastUpdate.Start();
	}

	void interpolationUpdate(vec2 serv_pos, float serv_angle) {
		initial_pos = position;
		initial_angle = angle;

		final_pos = serv_pos;
		final_angle = serv_angle;

		timeSinceLastUpdate.Start();
	}

	// Transform component
	vec2 position = vec2{ 0.0f, 0.0f };
	float angle = 0.0f;

	// Render component
	vec2 pivot = vec2{ 0.5f, 0.5f };
	vec2 size = vec2{ 0.0f, 0.0f }; // NOTE(jesus): If equals 0, it takes the size of the texture
	vec4 color = vec4{ 1.0f, 1.0f, 1.0f, 1.0f }; // NOTE(jesus): The texture will tinted with this color
	Texture * texture = nullptr;
	int  order = 0;          // NOTE(jesus): determines the drawing order

	// Collider component
	Collider *collider = nullptr;

	// "Script" component
	Behaviour *behaviour = nullptr;

	// Network identity component
	uint32 networkId = 0; // NOTE(jesus): Only for network game objects

	// NOTE(jesus): Don't use in gameplay systems (use Instantiate, Destroy instead)
	enum State { NON_EXISTING, CREATING, UPDATING, DESTROYING };
	State state = NON_EXISTING;

	// Tag for custom usage
	uint32 tag = ObjectType::EMPTY;
	uint32 team = ObjectTeam::NO_TEAM;


private:

	void * operator new(size_t size) = delete;
	void operator delete (void *obj) = delete;

	void releaseComponents();

	friend class ModuleGameObject;
};

class ModuleGameObject : public Module
{
public:

	// Virtual functions

	bool init() override;

	bool preUpdate() override;

	bool update() override;

	bool postUpdate() override;

	bool cleanUp() override;

	static GameObject * Instantiate();

	static void Destroy(GameObject * gameObject);

	void calculateInterpolation(uint32 not_update); // Modify position and angle based on initial, final and timeSinceLastUpdate



	GameObject gameObjects[MAX_GAME_OBJECTS] = {};
	float gameScale = 0.5;

	GameObject* spawnBackground(vec2 position, vec2 size);
};


// NOTE(jesus): These functions are named after Unity functions

GameObject *Instantiate();

void Destroy(GameObject *gameObject);
