#pragma once
#include "PushdownState.h"
#include <iostream>
#include "../../Common/Window.h"
#include "../GameTech/TutorialGame.h"
namespace NCL {
	namespace CSC8503 {
		class GameScreen : public PushdownState {
			PushdownResult OnUpdate(float dt, PushdownState **newState, TutorialGame* game) override {
				if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
					curOption = (curOption + 1) % 3;
				}
				
				if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
					curOption = (curOption - 1) % 3;
				}
				return PushdownResult::NoChange;
			};
			void OnAwake() override {

			}
		protected:
			int curOption = 0;
		};

	}
}
