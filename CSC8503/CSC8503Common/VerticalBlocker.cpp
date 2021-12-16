#include "VerticalBlocker.h"
#include "StateTransition.h"
#include "StateMachine.h"
#include "State.h"

using namespace NCL;
using namespace CSC8503;

VerticalBlocker::VerticalBlocker() {
	counter = 1.5f;
	stateMachine = new StateMachine();

	State* stateA = new State([&](float dt)-> void
	{
		this->MoveUp(dt);
	}
	);
	State* stateB = new State([&](float dt)-> void
	{
		this->MoveDown(dt);
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

VerticalBlocker ::~VerticalBlocker() {
	delete stateMachine;
}

void VerticalBlocker::Update(float dt) {
	stateMachine->Update(dt);
}

void VerticalBlocker::MoveUp(float dt) {
	GetPhysicsObject()->AddForce({ 0, -300, 0 });
	counter += dt;
}
void VerticalBlocker::MoveDown(float dt) {
	GetPhysicsObject()->AddForce({ 0, 300, 0 });
	counter -= dt;
}
