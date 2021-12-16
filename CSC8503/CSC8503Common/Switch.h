#pragma once
#include "GameObject.h"
#include "Spring.h"

namespace NCL {
	namespace CSC8503 {
		class Switch : public GameObject {
		public:
			Switch(std::string name = "Switch") : GameObject(name) {
				layer = Layer::StaticObjects;
			};
			
			virtual void OnSelect() override{
				spring->ToggleSpringCoil();
			}

			void SetSpring(Spring* spr) {
				spring = spr;
			}


		protected:
			Spring* spring;
		};
	}
}
