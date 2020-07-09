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
			float32x3 _up = { 0.0f, 0.0f, 1.0f }, float32 _fov = 1.0f, float32 _zNear = 0.1f, float32 _zFar = 100.0f)
		{
			position = _position;
			forward = _forward;
			up = _up;
			fov = _fov;
			zNear = _zNear;
			zFar = _zFar;
		}

		inline void translate(const float32x3& translation) { position += translation; }

		inline XLib::Matrix4x4 getViewMatrix() const { return XLib::Matrix4x4::LookAtCentered(position, forward, up); }
		inline XLib::Matrix4x4 getProjectionMatrix(float32 aspect) const { return XLib::Matrix4x4::Perspective(fov, aspect, zNear, zFar); }
	};

	struct XERCameraRotation
	{
		float32x2 angles = { 0.0f, 0.0f };		

		inline void setCameraForward(XERCamera& camera)
		{
			camera.forward = XLib::VectorMath::SphericalCoords_zZenith_xReference(angles);
		}

		// x - horizontal translation in xy plane (polar coords)
		// y - vertiacal translation (by camera up vector)
		// z - forward translation (spherical coords)
		inline void setCameraForwardAndTranslateRotated_cameraVertical(XERCamera& camera, const float32x3& translation)
		{
			setCameraForward(camera);

			float32x2 xyPlaneRight = XLib::VectorMath::PolarCoords_xReference(angles.x + XLib::Math::Pi<float> / 2.0f);
			float32x3 absoluteTranslation = camera.forward * translation.z;
			absoluteTranslation.xy += xyPlaneRight * translation.x;
			absoluteTranslation.z += translation.y;
			camera.position += absoluteTranslation;
		}

		//inline void translateRotated_localVertical(XERCamera& camera, const float32x3& translation) { }
	};
}