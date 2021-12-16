#pragma once
#include "PushdownState.h"
#include <iostream>
#include "../GameTech/TutorialGame.h"
namespace NCL {
	namespace CSC8503 {
		class LevelOneState : public PushdownState {
		public:
			LevelOneState(TutorialGame* game) : game(game) {};

			PushdownResult OnUpdate(float dt, PushdownState** newState) override {
				if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::ESCAPE) || game->goalReached || game->gameLost) {
					return PushdownResult::Pop; 
				}


				game->UpdateGame(dt);
				return PushdownResult::NoChange;
			};
			void OnAwake() override {
				game->InitLevel1();
			}
		protected:
			TutorialGame* game;
		};

	}
}
