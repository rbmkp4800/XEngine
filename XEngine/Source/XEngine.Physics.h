#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Vectors.h>
#include <XLib.Math.Quaternion.h>
#include <XLib.Containers.Vector.h>

namespace XEngine::Physics
{
	/*class CollisionShape
	{

	};*/

	class SphereCollisionShape
	{
	private:
		float32 radius;
	};

	class InfinitePlaneCollisionShape
	{
	private:
		float32x3 normal;
	};

	class RigidBody
	{
		friend class World;

	private:
		float32x3 position;
		float32x3 velocity;
		XLib::Quaternion orientation;
		float32x3 rotation;
		float32 inverseMass;

		inline void update(float32 duration, const float32x3& force);

	public:
		RigidBody() = default;
		~RigidBody() = default;

		// inline RigidBody(const float32x3& position, ...) : position(position) {}

		inline float32x3 getPosition() const { return position; }
		inline void setPosition(const float32x3& position) { this->position = position; }
		inline float32x3 getVelocity() const { return velocity; }
		inline void setVelocity(const float32x3& velocity) { this->velocity = velocity; }

		inline float32 getInverseMass() const { return inverseMass; }
		inline void setInverseMass(float32 inverseMass) { this->inverseMass = inverseMass; }
		inline float32 getMass() const { return 1.0f / inverseMass; }
		inline void setMass(float32 mass) { inverseMass = 1.0f / mass; }
	};

	class World : public XLib::NonCopyable
	{
	private:
		XLib::Vector<RigidBody*> rigidBodies;
		float32x3 gravity = { 0.0f, 0.0f, 0.0f };

	public:
		void update(float32 duration);
		
		void addRigidBody(RigidBody* rigidBody);

		inline void setGravity(const float32x3& gravity) { this->gravity = gravity; }
		inline float32x3 getGravity() const { return gravity; }
	};
}