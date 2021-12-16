#include "Enemy.h"
#include "StateTransition.h"
#include "StateMachine.h"
#include "State.h"

using namespace NCL;
using namespace CSC8503;

Enemy::Enemy(std::vector<Vector3>& pathNodes) : pathNodes(pathNodes) {
	counter = 1.0f;
	pathFindingTarget = nullptr;
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
			return false;
		}
	));

	stateMachine->AddTransition(new StateTransition(stateB, stateA,
		[&]()-> bool
		{
			return true;
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
	if (pathNodes.size() == 0 || curNode >= pathNodes.size()) {
		if (canSeePlayer && Vector3::Distance(GetTransform().GetPosition(), pathFindingTarget->GetTransform().GetPosition()) < 10) {
			Vector3 force = (GetTransform().GetPosition() - pathFindingTarget->GetTransform().GetPosition()).Normalised() * moveSpeed * dt;
			GetPhysicsObject()->AddForce(force);
		}
		return;
	}

	if (!(currentNodePos == pathNodes[curNode])) {
		curNode = 0;
		currentNodePos = pathNodes[0];
	}

	if (Vector3::Distance(this->GetTransform().GetPosition(), currentNodePos) < 15) {
		curNode++;
		currentNodePos = pathNodes[curNode];
	}

	Vector3 force = (currentNodePos - this->GetTransform().GetPosition()).Normalised() * moveSpeed * dt;

	GetPhysicsObject()->AddForce(force);

	counter += dt;
}

void Enemy::GoToPowerup(float dt) {
	counter -= dt;
}