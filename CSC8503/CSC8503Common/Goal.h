#pragma once
#include "GameObject.h"

namespace NCL {
	namespace CSC8503 {
		class Goal : public GameObject {
		public:
			Goal(std::string name = "Goal") : GameObject(name) {
				layer = Layer::StaticObjects | Layer::AntiGravity;
			};
			virtual void OnCollisionBegin(GameObject* otherObject) override {
			}
		};
	}
}
