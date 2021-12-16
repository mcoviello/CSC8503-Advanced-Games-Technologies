#pragma once
#include "PushdownState.h"
#include <iostream>
#include "../GameTech/TutorialGame.h"
namespace NCL {
	namespace CSC8503 {
		class LevelTwoState : public PushdownState {
		public:
			LevelTwoState(TutorialGame* game) : game(game) {};

			PushdownResult OnUpdate(float dt, PushdownState** newState) override {
				if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::ESCAPE) || game->goalReached || game->gameLost) {
					return PushdownResult::Pop;
				}

				game->UpdateGame(dt);
				return PushdownResult::NoChange;
			};
			void OnAwake() override {
				game->InitLevel2();
			}
			TutorialGame* game;
		};

	}
}
