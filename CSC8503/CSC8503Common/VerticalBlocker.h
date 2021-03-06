#pragma once
#include "StateGameObject.h"
namespace NCL {
	namespace CSC8503 {
		class StateMachine;
		class VerticalBlocker : public StateGameObject {
		public:
			VerticalBlocker();
			virtual ~VerticalBlocker();

			virtual void Update(float dt) override;

		protected:
			void MoveUp(float dt);
			void MoveDown(float dt);

			StateMachine* stateMachine;
			float counter;

		};

	}

}