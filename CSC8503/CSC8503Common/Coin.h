#pragma once
#include "GameObject.h"

namespace NCL {
	namespace CSC8503 {
		class Coin : public GameObject {
		public:
			Coin() {
				layer = Layer::Collectable;
			};
			virtual void OnCollisionBegin(GameObject* otherObject) override {
				if (otherObject->GetName() == "Player") {
					isActive = false;
				}
			}
		};
	}
}
