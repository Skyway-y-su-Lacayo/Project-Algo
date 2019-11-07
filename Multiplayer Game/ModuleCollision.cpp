#include "Networks.h"


static bool collisionTestOverSeparatingAxis(
	vec2 a1, vec2 a2, vec2 a3, vec2 a4, // Points in box a
	vec2 b1, vec2 b2, vec2 b3, vec2 b4, // Points in box b
	vec2 axis)                          // Separating axis test
{
	float pa1 = dot(a1, axis);
	float pa2 = dot(a2, axis);
	float pa3 = dot(a3, axis);
	float pa4 = dot(a4, axis);
	float pb1 = dot(b1, axis);
	float pb2 = dot(b2, axis);
	float pb3 = dot(b3, axis);
	float pb4 = dot(b4, axis);
	float maxa = max(pa1, max(pa2, max(pa3, pa4)));
	float mina = min(pa1, min(pa2, min(pa3, pa4)));
	float maxb = max(pb1, max(pb2, max(pb3, pb4)));
	float minb = min(pb1, min(pb2, min(pb3, pb4)));
	bool separated = maxa < minb || mina > maxb;
	return !separated;
}

static bool collisionTest(Collider &c1, Collider &c2)
{
	GameObject *a = c1.gameObject;
	GameObject *b = c2.gameObject;

	vec2 asize = isZero(a->size) ? (a->texture ? a->texture->size : vec2{ 100.0f, 100.0f }) : a->size;
	vec2 bsize = isZero(b->size) ? (b->texture ? b->texture->size : vec2{ 100.0f, 100.0f }) : b->size;

	mat4 aWorldMatrix =
		translation(a->position) *
		rotationZ(radiansFromDegrees(a->angle)) *
		scaling(asize) *
		translation(vec2{ 0.5f, 0.5f } -a->pivot);

	mat4 bWorldMatrix =
		translation(b->position) *
		rotationZ(radiansFromDegrees(b->angle)) *
		scaling(bsize) *
		translation(vec2{ 0.5f, 0.5f } -b->pivot);

	vec2 pa1 = vec2_cast(aWorldMatrix * vec4{ -0.5f, -0.5f, 0.0f, 1.0f });
	vec2 pa2 = vec2_cast(aWorldMatrix * vec4{  0.5f, -0.5f, 0.0f, 1.0f });
	vec2 pa3 = vec2_cast(aWorldMatrix * vec4{  0.5f,  0.5f, 0.0f, 1.0f });
	vec2 pa4 = vec2_cast(aWorldMatrix * vec4{ -0.5f,  0.5f, 0.0f, 1.0f });
	vec2 pb1 = vec2_cast(bWorldMatrix * vec4{ -0.5f, -0.5f, 0.0f, 1.0f });
	vec2 pb2 = vec2_cast(bWorldMatrix * vec4{  0.5f, -0.5f, 0.0f, 1.0f });
	vec2 pb3 = vec2_cast(bWorldMatrix * vec4{  0.5f,  0.5f, 0.0f, 1.0f });
	vec2 pb4 = vec2_cast(bWorldMatrix * vec4{ -0.5f,  0.5f, 0.0f, 1.0f });

	vec2 axes[] = {	pa1 - pa2, pa2 - pa3, pb1 - pb2, pb2 - pb3 };

	for (vec2 axis : axes)
	{
		if (!collisionTestOverSeparatingAxis(pa1, pa2, pa3, pa4, pb1, pb2, pb3, pb4, axis))
		{
			return false;
		}
	}

	return true;
}


Collider * ModuleCollision::addCollider(ColliderType type, GameObject * parent)
{
	ASSERT(type != ColliderType::None);

	for (Collider &collider : colliders)
	{
		if (collider.type == ColliderType::None)
		{
			collider.type = type;
			collider.gameObject = parent;
			activeCollidersCount++;
			return &collider;
		}
	}

	ASSERT(0); // No space for more colliders, increase MAX_COLLIDERS

	return nullptr;
}

void ModuleCollision::removeCollider(Collider *collider)
{
	if (collider->type != ColliderType::None)
	{
		collider->type = ColliderType::None;
		collider->gameObject = nullptr;
		activeCollidersCount--;
	}
}

bool ModuleCollision::update()
{
	// Pack colliders in activeColliders without empty cells
	uint32 activeColliderIndex = 0;
	for (int i = 0; i < MAX_COLLIDERS && activeColliderIndex < activeCollidersCount; ++i)
	{
		Collider *collider = &colliders[i];

		if (collider->type != ColliderType::None)
		{
			activeColliders[activeColliderIndex++] = collider;
		}
	}

	// Traverse all active colliders
 	for (uint32 i = 0; i < activeCollidersCount; ++i)
	{
		Collider &c1 = *activeColliders[i];

		if (c1.type != ColliderType::None)
		{
			for (uint32 j = i + 1; j < activeCollidersCount; ++j)
			{
				Collider &c2 = *activeColliders[j];

				if (c2.type != ColliderType::None)
				{
					if ((c1.isTrigger && c1.gameObject->behaviour != nullptr) ||
						(c2.isTrigger && c2.gameObject->behaviour != nullptr))
					{
						if (collisionTest(c1, c2))
						{
							if (c1.isTrigger)
							{
								c1.gameObject->behaviour->onCollisionTriggered(c1, c2);
							}
							if (c2.isTrigger)
							{
								c2.gameObject->behaviour->onCollisionTriggered(c2, c1);
							}
						}
					}
				}
			}
		}
	}

	return true;
}

bool ModuleCollision::postUpdate()
{
	return true;
}
