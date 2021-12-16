#include "Enemy.h"
#include "StateTransition.h"
#include "StateMachine.h"
#include "State.h"

using namespace NCL;
using namespace CSC8503;

Enemy::Enemy() {
	counter = 1.0f;
	stateMachine = new StateMachine();

	State* stateA = new State([&](float dt)-> void
		{
			this->ChasePlayer(dt);
		}
	);
	State* stateB = new State([&](float dt)-> void
		{
			this->GoToPowerup(dt);
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

Enemy ::~Enemy() {
	delete stateMachine;
}

void Enemy::Update(float dt) {
	stateMachine->Update(dt);
}

void Enemy::ChasePlayer(float dt) {

}

void Enemy::GoToPowerup(float dt) {

}