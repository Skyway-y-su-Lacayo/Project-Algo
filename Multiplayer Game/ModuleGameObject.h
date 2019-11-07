#pragma once

enum ObjectType {
	EMPTY,
	LASER,
	SPACESHIP
};
struct GameObject
{
	// Transform component
	vec2 position = vec2{ 0.0f, 0.0f };

	// Render component
	vec2 pivot = vec2{ 0.5f, 0.5f };
	vec2 size = vec2{ 0.0f, 0.0f }; // NOTE(jesus): If equals 0, it takes the size of the texture
	float angle = 0.0f;
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



	GameObject gameObjects[MAX_GAME_OBJECTS] = {};
};


// NOTE(jesus): These functions are named after Unity functions

GameObject *Instantiate();

void Destroy(GameObject *gameObject);
