#include "Enemy.h"
#include "StateTransition.h"
#include "StateMachine.h"
#include "State.h"
#include "PlayerObj.h"

using namespace NCL;
using namespace CSC8503;

Enemy::Enemy(std::vector<Vector3>& pathNodes, PlayerObj* player) : pathNodes(pathNodes) {
	name = "Enemy";
	pathFindingTarget = nullptr;
	stateMachine = new StateMachine();

	State* stateA = new State([&](float dt)-> void
		{
			this->GoToTarget(dt);
		}
	);
	State* stateB = new State([&](float dt)-> void
		{
			this->HoneIn(dt);
		}
	);

	stateMachine->AddState(stateA);
	stateMachine->AddState(stateB);

	stateMachine->AddTransition(new StateTransition(stateA, stateB,
		[&]()-> bool
		{
			//When the enemy has no idea where the player is, or there is a bonus around the stage
		return Vector3::Distance(pathFindingTarget->GetTransform().GetPosition(), GetTransform().GetPosition()) < 20;
		}
	));

	stateMachine->AddTransition(new StateTransition(stateB, stateA,
		[&]()-> bool
		{
			//When the enemy spots the player
			return Vector3::Distance(pathFindingTarget->GetTransform().GetPosition(), GetTransform().GetPosition()) >= 20;
		}
	));
}

Enemy ::~Enemy() {
	delete stateMachine;
}

void Enemy::Update(float dt) {
	stateMachine->Update(dt);

}

void Enemy::GoToTarget(float dt) {
	//std::cout << "Walking to Target\n";
	if (pathNodes.size() == 0 || curNode >= pathNodes.size()-1) {
		return;
	}

	currentNodePos = pathNodes[curNode];

	if (!(currentNodePos == pathNodes[curNode])) {
		curNode = 0;
		currentNodePos = pathNodes[0];
	}

	if (Vector3::Distance(this->GetTransform().GetPosition(), currentNodePos) < 15) {
		curNode++;
	}

	Vector3 force = (currentNodePos - this->GetTransform().GetPosition()).Normalised() * moveSpeed * dt;

	GetPhysicsObject()->AddForce(force);
}

void Enemy::HoneIn(float dt) {
	//std::cout << "Close Chase\n";
	Vector3 force = (pathFindingTarget->GetTransform().GetPosition() - GetTransform().GetPosition()).Normalised() * moveSpeed * dt;
	GetPhysicsObject()->AddForce(force);
}