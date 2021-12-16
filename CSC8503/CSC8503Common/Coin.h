#pragma once
#include "GameObject.h"

namespace NCL {
	namespace CSC8503 {
		class Coin : public GameObject {
		public:
			Coin(std::string name = "Coin") : GameObject(name){
				layer = Layer::DontResolveCollisions | Layer::AntiGravity;
			};
			virtual void OnCollisionBegin(GameObject* otherObject) override {
				if (otherObject->GetName() == "Player") {
					//Hide the coin, ignore all subsequent collisions
					isActive = false;
					layer = Layer::IgnoreAllCollisions;
				}
			}
		};
	}
}
