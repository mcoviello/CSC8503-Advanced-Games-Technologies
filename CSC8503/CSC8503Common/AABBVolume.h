#pragma once
#include "CollisionVolume.h"
#include "../../Common/Vector3.h"
namespace NCL {
	class AABBVolume : CollisionVolume
	{
	public:
		AABBVolume(const Vector3& halfDims) {
			type		= VolumeType::AABB;
			halfSizes	= halfDims;
		}
		~AABBVolume() {
		}

		Vector3 SupportFunction(const Transform& worldTransform, Vector3 axis) const override {
			Vector3 vertex;
			vertex.x = axis.x < 0 ? -1.0f : 1.0f;
			vertex.y = axis.y < 0 ? -1.0f : 1.0f;
			vertex.z = axis.z < 0 ? -1.0f : 1.0f;
			return worldTransform.GetMatrix() * vertex;
		}

		Vector3 GetHalfDimensions() const {
			return halfSizes;
		}

	protected:
		Vector3 halfSizes;
	};
}
