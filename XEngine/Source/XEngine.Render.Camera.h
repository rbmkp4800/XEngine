#pragma once

#include <XLib.Types.h>
#include <XLib.Vectors.h>
#include <XLib.Vectors.Arithmetics.h>
#include <XLib.Vectors.Math.h>
#include <XLib.Math.Matrix4x4.h>

namespace XEngine
{
	struct XERCamera
	{
		float32x3 position, forward, up;
		float32 fov, zNear, zFar;

		inline XERCamera(float32x3 _position = { 0.0f, 0.0f, 0.0f }, float32x3 _forward = { 0.0f, 0.0f, 1.0f },
			float32x3 _up = { 0.0f, 1.0f, 0.0f }, float32 _fov = 1.0f, float32 _zNear = 0.1f, float32 _zFar = 100.0f)
		{
			position = _position;
			forward = _forward;
			up = _up;
			fov = _fov;
			zNear = _zNear;
			zFar = _zFar;
		}

		inline void translate(const float32x3& translation) { position += translation; }
		inline void translateInViewSpace(const float32x3& translation)
		{
			// TODO: use up vector

			float32x2 right = XLib::VectorMath::Normalize(float32x2(forward.z, -forward.x));

			float32x3 absoluteTranslation = XLib::VectorMath::Normalize(forward) * translation.z;
			absoluteTranslation.x += right.x * translation.x;
			absoluteTranslation.z += right.y * translation.x;
			absoluteTranslation.y += translation.y;
			position += absoluteTranslation;

			/*float32x2 right = XLib::VectorMath::PolarCoords(rotation.x + XLib::Math::PiF32 / 2.0f);
			position += target * moveVector.z;
			position.x += right.x * moveVector.x;
			position.z += right.y * moveVector.x;
			position.y += moveVector.y;*/
		}
		inline void rotate(float32x2 angle)
		{
			// TODO: use up vector

			/*float32x3 forward = target - position;
			float32x2 right = { forward.z, -forward.x };

			float32x4 newForward = forward * XLib::Matrix4x4::Ro
			//XLib::Matrix4x4::RotationY(-angle.x);
			target = position + newForward.xyz;*/
		}

		inline XLib::Matrix4x4 getViewMatrix() const { return XLib::Matrix4x4::LookAtCentered(position, forward, up); }
		inline XLib::Matrix4x4 getProjectionMatrix(float32 aspect) const { return XLib::Matrix4x4::Perspective(fov, aspect, zNear, zFar); }
	};
}