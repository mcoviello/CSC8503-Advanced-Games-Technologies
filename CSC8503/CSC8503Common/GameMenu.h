#pragma once
#include "PushdownState.h"
#include <iostream>
#include "../../Common/Window.h"
#include "LevelOneState.h"
#include "LevelTwoState.h"
#include <iostream>;
namespace NCL {
	namespace CSC8503 {
		class GameMenu : public PushdownState {
		public:
			GameMenu(TutorialGame* game) : game(game) {};
			PushdownResult OnUpdate(float dt, PushdownState **newState) override {
				if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::UP)) {
					curOption = (curOption + 1) % noOfOptions;
				}
				
				if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::DOWN)) {
					curOption = abs((curOption - 1) % noOfOptions);
				}

				if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::RETURN)) {
					switch (curOption) {
					case 0:
						*newState = new LevelOneState(game);
						return PushdownResult::Push;
					case 1:
						*newState = new LevelTwoState(game);
						return PushdownResult::Push;
					}
				}

				game->Menu(curOption, dt);
				return PushdownResult::NoChange;
			};

			void OnAwake() override {
			};
		protected:
			int curOption = 0;
			int noOfOptions = 2;
			TutorialGame* game;
		};

	}
}
