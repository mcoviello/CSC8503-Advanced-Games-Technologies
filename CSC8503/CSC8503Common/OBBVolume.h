#pragma once
#include "CollisionVolume.h"
#include "../../Common/Vector3.h"
namespace NCL {
	class OBBVolume : CollisionVolume
	{
	public:
		OBBVolume(const Maths::Vector3& halfDims) {
			type		= VolumeType::OBB;
			halfSizes	= halfDims;
		}
		~OBBVolume() {}

		Vector3 SupportFunction(const Transform& worldTransform, Vector3 axis) const override {
			Vector3 localAxis = worldTransform.GetOrientation().Conjugate() * axis;
			Vector3 vertex;
			vertex.x = localAxis.x < 0 ? -0.5f : 0.5f;
			vertex.y = localAxis.y < 0 ? -0.5f : 0.5f;
			vertex.z = localAxis.z < 0 ? -0.5f : 0.5f;
			return worldTransform.GetMatrix() * vertex;
		}

		Maths::Vector3 GetHalfDimensions() const {
			return halfSizes;
		}
	protected:
		Maths::Vector3 halfSizes;
	};
}

