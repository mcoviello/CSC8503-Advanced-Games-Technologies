#pragma once
#include "StateGameObject.h"

namespace NCL {
	namespace CSC8503 {
		class Enemy : public StateGameObject {
		public:
			Enemy(std::vector<Vector3>& pathNodes);
			virtual ~Enemy();
			virtual void OnCollisionBegin(GameObject* otherObject) override {
				if (otherObject->GetName() == "Player") {
					
				}

				if (otherObject->GetName() == "Coin") {
					moveSpeed += 10.0f;
				}
			}

			GameObject* GetTarget() { return pathFindingTarget; }
			void SetTarget(GameObject* target) { this->pathFindingTarget = target; }

			virtual void Update(float dt) override;
		protected:

			void ChasePlayer(float dt);
			void GoToPowerup(float dt);

			GameObject* pathFindingTarget;
			bool canSeePlayer;

			StateMachine* stateMachine;
			float counter;

			std::vector<Vector3>& pathNodes;
			Vector3 currentNodePos;
			int curNode;

			float defaultMoveSpeed = 100.0f;
			float moveSpeed = 2000;
		};
	}
}
