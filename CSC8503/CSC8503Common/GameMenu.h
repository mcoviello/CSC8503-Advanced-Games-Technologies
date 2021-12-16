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
				if (game->goalReached) {
					game->ShowScore(dt);


					if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::ESCAPE)) {
						game->goalReached = false;
					}
				}
				else {
					if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::UP)) {
						curOption = curOption - 1 < 0 ? noOfOptions - 1 : curOption - 1;
					}

					if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::DOWN)) {
						curOption = (curOption + 1) % noOfOptions;
					}

					if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::RETURN)) {
						switch (curOption) {
						case 0:
							*newState = new LevelOneState(game);
							return PushdownResult::Push;
						case 1:
							*newState = new LevelTwoState(game);
							return PushdownResult::Push;
						case 2:
							game->exitGame = true;
						}
					}

					if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::ESCAPE)) {
						game->exitGame = true;
					}

					game->Menu(curOption, dt);
				}
				return PushdownResult::NoChange;
			};

			void OnAwake() override {
			};
		protected:
			int curOption = 0;
			int noOfOptions = 3;
			TutorialGame* game;
		};

	}
}
