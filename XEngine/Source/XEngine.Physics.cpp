#include <XLib.Vectors.Arithmetics.h>

#include "XEngine.Physics.h"

using namespace XLib;
using namespace XEngine::Physics;

inline void RigidBody::update(float32 duration, const float32x3& force)
{
	float32x3 acceleration = force * inverseMass;
	velocity += acceleration * duration;
	position += velocity * duration;
}

void World::update(float32 duration)
{
	for each (RigidBody* body in rigidBodies)
		body->update(duration, gravity);

	// generate contacts

	// resolve contacts
}

void World::addRigidBody(RigidBody* rigidBody)
{
	rigidBodies.pushBack(rigidBody);
}