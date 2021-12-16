#pragma once
#include "GameObject.h"

namespace NCL {
	namespace CSC8503 {
		class PlayerObj : public GameObject {
		public:
			PlayerObj(std::string name = "Player") : GameObject(name) {
				layer = Layer::Player;
			};

			virtual void OnCollisionBegin(GameObject* otherObject) override {
				if (otherObject->GetName() == "Coin") {
					score += 10;
				}

				if (otherObject->GetName() == "Goal") {
					goalReached = true;
				}
			}

			void ResetScore() {
				score = 0;
			}

			void SetScore(int newScore) {
				score = newScore;
			}

			int GetScore() {
				return score;
			}

			bool GoalReached() {
				return goalReached;
			}
		protected:
			int score = 0;
			bool goalReached = false;
		};
	}
}

