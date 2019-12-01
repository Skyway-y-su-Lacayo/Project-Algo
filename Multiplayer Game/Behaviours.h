#pragma once

#include "ModuleNetworkingServer.h"
#include "Animation.h"


vec2 boundaries = { 500,500 };

struct Behaviour
{
	GameObject *gameObject = nullptr;

	virtual void start() { }

	virtual void update() { }

	virtual void onInput(const InputController &input, const MouseController &mouse) { }

	virtual void onCollisionTriggered(Collider &c1, Collider &c2) { }

	virtual ~Behaviour() {}
};

struct Player : public Behaviour
{
	uint16 lives = 4;
	const float speed = 200.0f;
};

// Movement: WASD / PAD + Mouse for orientation
// Action: Right click shoots a basic bullet
struct Shooter : public Player
{
	void start() override
	{

	}

	void onInput(const InputController &input, const MouseController &mouse) override
	{

		float normalizedSpeed = speed * Time.deltaTime;

		if (input.horizontalAxis < -0.01f)
			gameObject->position.x -= normalizedSpeed;

		if (input.horizontalAxis > 0.01f)
			gameObject->position.x += normalizedSpeed;

		if (input.verticalAxis < -0.01f)
			gameObject->position.y += normalizedSpeed;

		if (input.verticalAxis > 0.01f)
			gameObject->position.y -= normalizedSpeed;

		// Boundaries

		if (gameObject->position.x < -boundaries.x)
			gameObject->position.x = -boundaries.y;

		if (gameObject->position.x > boundaries.x)
			gameObject->position.x = boundaries.x;

		if (gameObject->position.y < -boundaries.y)
			gameObject->position.y = -boundaries.y;

		if (gameObject->position.y > boundaries.y) 
			gameObject->position.y = boundaries.y;

		NetworkUpdate(gameObject);

		// Angle of go depending on mouse position
		gameObject->angle = atan2(mouse.x, -mouse.y) * 180/PI;

		if (mouse.buttons[0] == ButtonState::Press)
			App->modNetServer->spawnBullet(gameObject, ColliderType::SoftLaser);
	}

	void onCollisionTriggered(Collider &c1, Collider &c2) override
	{

		if (c2.type == ColliderType::HardLaser && c2.gameObject->team != gameObject->team)
		{
			NetworkDestroy(c2.gameObject); // Destroy the laser
			lives -= 1;
			if (lives <= 0){
				App->modNetServer->destroyClientProxyByGO(c1.gameObject);
				lives = 0;
			}

			
			// NOTE(jesus): spaceship was collided by a laser
			// Be careful, if you do NetworkDestroy(gameObject) directly,
			// the client proxy will poing to an invalid gameObject...
			// instead, make the gameObject invisible or disconnect the client.
		}
	}



};

struct ShooterClient : public Player
{
	ShooterClient(GameObject* go) { gameObject = go; };

	void onInput(const InputController &input, const MouseController &mouse) override
	{
		float normalizedSpeed = speed * Time.deltaTime;

		if (input.horizontalAxis < -0.01f)
			gameObject->position.x -= normalizedSpeed;

		if (input.horizontalAxis > 0.01f)
			gameObject->position.x += normalizedSpeed;

		if (input.verticalAxis < -0.01f)
			gameObject->position.y += normalizedSpeed;

		if (input.verticalAxis > 0.01f)
			gameObject->position.y -= normalizedSpeed;
		
		// Angle of go depending on mouse position
		gameObject->angle = atan2(mouse.x, -mouse.y) * 180 / PI;
	}
};
// Movement: WASD / PAD + Mouse for orientation+
// Dash?: Spacebar
// Action: Has a reflector in front of him with the same orientation
struct Reflector : public Player {

	GameObject* reflector_barrier = nullptr;
	float barrier_offset = 50;

	void start() override {

		// New reflector
		reflector_barrier = App->modNetServer->spawnReflectorBarrier(gameObject);

	}

	void onInput(const InputController &input, const MouseController &mouse) override {

		float normalizedSpeed = speed * Time.deltaTime;

		if (input.horizontalAxis < -0.01f)
			gameObject->position.x -= normalizedSpeed;

		if (input.horizontalAxis > 0.01f)
			gameObject->position.x += normalizedSpeed;

		if (input.verticalAxis < -0.01f)
			gameObject->position.y += normalizedSpeed;

		if (input.verticalAxis > 0.01f)
			gameObject->position.y -= normalizedSpeed;


		// Boundaries

		if (gameObject->position.x < -boundaries.x)
			gameObject->position.x = -boundaries.y;

		if (gameObject->position.x > boundaries.x)
			gameObject->position.x = boundaries.x;

		if (gameObject->position.y < -boundaries.y)
			gameObject->position.y = -boundaries.y;

		if (gameObject->position.y > boundaries.y)
			gameObject->position.y = boundaries.y;

		NetworkUpdate(gameObject);
		NetworkUpdate(reflector_barrier);

		// Angle of go depending on mouse position
		gameObject->angle = atan2(mouse.x, -mouse.y) * 180 / PI;


		// Update reflection barrier
		vec2 forward = { mouse.x, mouse.y };
		forward = normalize(forward);
		reflector_barrier->position = gameObject->position + forward * barrier_offset;
		reflector_barrier->angle = gameObject->angle;

	}

	void onCollisionTriggered(Collider &c1, Collider &c2) override {

		if (c2.type == ColliderType::HardLaser && c2.gameObject->team != gameObject->team) {
			NetworkDestroy(c2.gameObject); // Destroy the laser
			lives -= 1;
			if (lives <= 0) {
				App->modNetServer->destroyClientProxyByGO(c1.gameObject);
				lives = 0;
			}

			// NOTE(jesus): spaceship was collided by a laser
			// Be careful, if you do NetworkDestroy(gameObject) directly,
			// the client proxy will poing to an invalid gameObject...
			// instead, make the gameObject invisible or disconnect the client.
		}
	}

	~Reflector() {
		NetworkDestroy(reflector_barrier);
	}
};

struct ReflectorClient : public Player
{
	GameObject* reflector_barrier = nullptr;
	float barrier_offset = 50;

	ReflectorClient(GameObject* go) { gameObject = go; };
	void onInput(const InputController &input, const MouseController &mouse) override {

		float normalizedSpeed = speed * Time.deltaTime;

		if (input.horizontalAxis < -0.01f)
			gameObject->position.x -= normalizedSpeed;

		if (input.horizontalAxis > 0.01f)
			gameObject->position.x += normalizedSpeed;

		if (input.verticalAxis < -0.01f)
			gameObject->position.y += normalizedSpeed;

		if (input.verticalAxis > 0.01f)
			gameObject->position.y -= normalizedSpeed;

		// Angle of go depending on mouse position
		gameObject->angle = atan2(mouse.x, -mouse.y) * 180 / PI;

		if (reflector_barrier)
		{
			vec2 forward = { mouse.x, mouse.y };
			forward = normalize(forward);
			reflector_barrier->position = gameObject->position + forward * barrier_offset;
			reflector_barrier->angle = gameObject->angle;
		}
		// Update reflection barrier
		//vec2 forward = { mouse.x, mouse.y };
		//forward = normalize(forward);
		//reflector_barrier->position = gameObject->position + forward * barrier_offset;
		//reflector_barrier->angle = gameObject->angle;

	}
};

struct ReflectorBarrier : public Behaviour {

	bool create_bullet = false;

	void update() override {
		if (create_bullet) {
			create_bullet = false;
			GameObject* Bullet = App->modNetServer->spawnBullet(gameObject, ColliderType::HardLaser);
		}
	}
	void onCollisionTriggered(Collider &c1, Collider &c2) override {
		if (c2.type == ColliderType::SoftLaser && c1.gameObject->team == c2.gameObject->team) {
			NetworkDestroy(c2.gameObject); // Destroy the laser
			create_bullet = true; // Can't create colliders in the middle of an iteration
		}
	}
};

struct Laser : public Behaviour
{
	float secondsSinceCreation = 0.0f;

	void update() override
	{
		const float pixelsPerSecond = 1000.0f;
		gameObject->position += vec2FromDegrees(gameObject->angle) * pixelsPerSecond * Time.deltaTime;

		secondsSinceCreation += Time.deltaTime;

		NetworkUpdate(gameObject);

		const float lifetimeSeconds = 2.0f;
		if (secondsSinceCreation > lifetimeSeconds) NetworkDestroy(gameObject);
	}
};

struct ClientLaser : public Behaviour 	
{
	void update() override {
		const float pixelsPerSecond = 1000.0f;
		gameObject->position += vec2FromDegrees(gameObject->angle) * pixelsPerSecond * Time.deltaTime;
	}
};

struct PlayerHealth : public Behaviour 	
{
	GameObject* player = nullptr;
	float yOffset = -100;

	void start() override {
		gameObject->currentAnimation = new Animation();
		gameObject->currentAnimation->pushFrame({ 0,0,164,34 });
		gameObject->currentAnimation->pushFrame({ 0,34,164,34 });
		gameObject->currentAnimation->pushFrame({ 0,68,164,34 });
		gameObject->currentAnimation->pushFrame({ 0,102,164,34 });
		gameObject->currentAnimation->setStatic(true);
	}

	void update() override {
		gameObject->position = player->position;
		gameObject->position.y += yOffset;
		Player* behaviour = (Player*)player->behaviour;
		gameObject->currentAnimation->setFrame(4 - behaviour->lives);
	}
};