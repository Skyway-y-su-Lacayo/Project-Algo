#pragma once

#include "ModuleNetworkingServer.h"
struct Behaviour
{
	GameObject *gameObject = nullptr;

	virtual void start() { }

	virtual void update() { }

	virtual void onInput(const InputController &input, const MouseController &mouse) { }

	virtual void onCollisionTriggered(Collider &c1, Collider &c2) { }
};

struct Spaceship : public Behaviour
{
	uint32 lives = 3;
	const float speed = 200.0f;

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

		NetworkUpdate(gameObject);

		// Angle of go depending on mouse position
		gameObject->angle = atan2(mouse.x, -mouse.y) * 180/PI;

		if (mouse.buttons[0] == ButtonState::Press)
			App->modNetServer->spawnBullet(gameObject);
	}

	void onCollisionTriggered(Collider &c1, Collider &c2) override
	{

		if (c2.type == ColliderType::Laser && c2.gameObject->team != gameObject->team)
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
