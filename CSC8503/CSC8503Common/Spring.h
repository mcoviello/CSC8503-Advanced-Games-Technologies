#pragma once
#include "GameObject.h"
#include "../CSC8503Common/Constraint.h"
namespace NCL {
	namespace CSC8503 {

		class Spring : public Constraint
		{
		public:
			Spring(GameObject* a, GameObject* b, float restingLength = 0.0f, float k = 1.0f, bool coiled = true) : obj1(a), obj2(b),
				restingLength(restingLength), k(k), coiled(coiled) {}

			void UpdateConstraint(float dt) override {
				if (!obj1 || !obj2) {
					return;
				}
				float distBetween = Vector3::Distance(obj1->GetTransform().GetPosition(), obj2->GetTransform().GetPosition());
				float extension = coiled ? distBetween : distBetween - restingLength;

				Vector3 dirVec = (obj1->GetTransform().GetPosition() - obj2->GetTransform().GetPosition()).Normalised();

				obj1->GetPhysicsObject()->AddForce(dirVec * extension * -k * dt);
				obj2->GetPhysicsObject()->AddForce(-dirVec * extension * -k * dt);
			}

			void ToggleSpringCoil() {
				coiled = !coiled;
			}
		protected:
			GameObject* obj1;
			GameObject* obj2;
			float restingLength;
			float k;
			bool coiled;
		};
	}
}

