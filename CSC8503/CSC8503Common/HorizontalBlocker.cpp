#include "HorizontalBlocker.h"
#include "StateTransition.h"
#include "StateMachine.h"
#include "State.h"

using namespace NCL;
using namespace CSC8503;

HorizontalBlocker::HorizontalBlocker() {
	counter = 1.0f;
	stateMachine = new StateMachine();

	State* stateA = new State([&](float dt)-> void
		{
			this->MoveLeft(dt);
		}
	);
	State* stateB = new State([&](float dt)-> void
		{
			this->MoveRight(dt);
		}
	);

	stateMachine->AddState(stateA);
	stateMachine->AddState(stateB);

	stateMachine->AddTransition(new StateTransition(stateA, stateB,
		[&]()-> bool
		{
			return this->counter > 3.0f;
		}
	));

	stateMachine->AddTransition(new StateTransition(stateB, stateA,
		[&]()-> bool
		{
			return this->counter < 0.0f;
		}
	));
}

HorizontalBlocker ::~HorizontalBlocker() {
	delete stateMachine;
}

void HorizontalBlocker::Update(float dt) {
	stateMachine->Update(dt);
}

void HorizontalBlocker::MoveLeft(float dt) {
	GetPhysicsObject()->AddForce({0, 0,  -200 });
	counter += dt;
}
void HorizontalBlocker::MoveRight(float dt) {
	GetPhysicsObject()->AddForce({ 0, 0, 200 });
	counter -= dt;
}
