#pragma once
#include "GameObject.h"
namespace NCL {
	namespace CSC8503 {
		class StateMachine;
		class StateGameObject : public GameObject {
		public:
			StateGameObject() {};
			virtual ~StateGameObject() {};

			virtual void Update(float dt) {};
		};

	}

}