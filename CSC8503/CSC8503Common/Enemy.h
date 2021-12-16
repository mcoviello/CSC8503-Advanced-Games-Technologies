#pragma once
#include "StateGameObject.h"

namespace NCL {
	namespace CSC8503 {
		class Enemy : public StateGameObject {
		public:
			Enemy();
			virtual void OnCollisionBegin(GameObject* otherObject) override {
				if (otherObject->GetName() == "Player") {

				}

				if (otherObject->GetName() == "Coin") {
					moveSpeed += 10.0f;
				}
			}

			virtual void Update(float dt) override;
		protected:

			void ChasePlayer(float dt);
			void GoToPowerup(float dt);
			void RaycastToPlayer();

			GameObject* pathFindingTarget;
			GameObject* player;

			StateMachine* stateMachine;
			float counter;

			float defaultMoveSpeed = 100.0f;
			float moveSpeed = 100.0f;
		};
	}
}
