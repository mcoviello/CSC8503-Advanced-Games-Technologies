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
				//Dampen the forces of the spring to get rid of infinite oscillations
				Vector3 dampingForce1 = obj1->GetPhysicsObject()->GetLinearVelocity() * damping;
				Vector3 dampingForce2 = obj2->GetPhysicsObject()->GetLinearVelocity() * damping;

				obj1->GetPhysicsObject()->ApplyLinearImpulse(((dirVec * extension * -k) - dampingForce1)* dt);
				obj2->GetPhysicsObject()->ApplyLinearImpulse(-((dirVec * extension * -k) + dampingForce2) * dt);
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
			float damping = 5.0f;
		};
	}
}

