#pragma once
#include "CollisionVolume.h"

namespace NCL {
	class SphereVolume : CollisionVolume
	{
	public:
		SphereVolume(float sphereRadius = 1.0f) {
			type	= VolumeType::Sphere;
			radius	= sphereRadius;
		}
		~SphereVolume() {}

		Vector3 SupportFunction(const Transform& worldTransform, Vector3 axis) const override {
			return worldTransform.GetPosition() + (axis.Normalised() * radius);
		}

		float GetRadius() const {
			return radius;
		}
	protected:
		float	radius;
	};
}

