#pragma once
#include "GameObject.h"

namespace NCL {
	namespace CSC8503 {
		class Player : public GameObject {
		public:
			Player() {
				layer = Layer::Player;
			};
			virtual void OnCollisionBegin(GameObject* otherObject) override {
				if (otherObject->GetName() == "Coin") {
					score += 10;
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
		protected:
			int score = 0
		};
	}
}

