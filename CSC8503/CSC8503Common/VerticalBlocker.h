#pragma once
#include "GameObject.h"
namespace NCL {
	namespace CSC8503 {
		class StateMachine;
		class VerticalBlocker : public GameObject {
		public:
			VerticalBlocker();
			~VerticalBlocker();

			virtual void Update(float dt);

		protected:
			void MoveUp(float dt);
			void MoveDown(float dt);

			StateMachine* stateMachine;
			float counter;

		};

	}

}