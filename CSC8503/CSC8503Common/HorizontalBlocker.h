#pragma once
#include "StateGameObject.h"
namespace NCL {
	namespace CSC8503 {
		class StateMachine;
		class HorizontalBlocker : public StateGameObject {
		public:
			HorizontalBlocker();
			~HorizontalBlocker();

			virtual void Update(float dt) override;

		protected:
			void MoveLeft(float dt);
			void MoveRight(float dt);

			StateMachine* stateMachine;
			float counter;

		};

	}

}