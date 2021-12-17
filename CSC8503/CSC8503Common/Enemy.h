#pragma once
#include "StateGameObject.h"

namespace NCL {
	namespace CSC8503 {
		class PlayerObj;
		class Enemy : public StateGameObject {
		public:
			Enemy(std::vector<Vector3>& pathNodes, PlayerObj* player);
			virtual ~Enemy();
			virtual void OnCollisionBegin(GameObject* otherObject) override {
				//Allow for temporary movement boost
				if (otherObject->GetName() == "SpeedPowerup") {
					moveSpeed += 10.0f;
					speedTimer = 0;
				}
			}

			void SetCanSeePlayer(bool canSee) {
				canSeePlayer = canSee;
			}

			GameObject* GetTarget() { return pathFindingTarget; }
			void SetTarget(GameObject* target) { this->pathFindingTarget = target; }

			virtual void Update(float dt) override;
		protected:

			void GoToTarget(float dt);
			void HoneIn(float dt);

			PlayerObj* player;
			GameObject* pathFindingTarget;
			bool canSeePlayer;

			StateMachine* stateMachine;
			float speedTimer;

			std::vector<Vector3>& pathNodes;
			Vector3 currentNodePos;
			int curNode;

			float defaultMoveSpeed = 100.0f;
			float moveSpeed = 2000;
		};
	}
}
