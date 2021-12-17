#include "TutorialGame.h"
#include "../CSC8503Common/GameWorld.h"
#include "../../Plugins/OpenGLRendering/OGLMesh.h"
#include "../../Plugins/OpenGLRendering/OGLShader.h"
#include "../../Plugins/OpenGLRendering/OGLTexture.h"
#include "../../Common/TextureLoader.h"
#include"../CSC8503Common/PositionConstraint.h"
#include"../CSC8503Common/Spring.h"
#include"../CSC8503Common/Coin.h"
#include"../CSC8503Common/Goal.h"
#include"../CSC8503Common/Switch.h"
#include "../CSC8503Common/VerticalBlocker.h"
#include "../CSC8503Common/HorizontalBlocker.h"
#include "../CSC8503Common/PlayerObj.h"
#include "../CSC8503Common/Enemy.h"
#include "../CSC8503Common/PushdownState.h"
#include "../CSC8503Common/NavigationGrid.h"
#include "../CSC8503Common/NavigationPath.h"

using namespace NCL;
using namespace CSC8503;

TutorialGame::TutorialGame()	{
	world		= new GameWorld();
	renderer	= new GameTechRenderer(*world);
	physics		= new PhysicsSystem(*world);

	forceMagnitude	= 10.0f;
	useGravity		= false;
	inSelectionMode = false;
	validSpawnPositions = {Vector3(80,5,80), Vector3(120,5,140), Vector3(40,5,140) };

	Debug::SetRenderer(renderer);

	InitialiseAssets();
}

/*

Each of the little demo scenarios used in the game uses the same 2 meshes, 
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it if you like!

*/
void TutorialGame::InitialiseAssets() {
	auto loadFunc = [](const string& name, OGLMesh** into) {
		*into = new OGLMesh(name);
		(*into)->SetPrimitiveType(GeometryPrimitive::Triangles);
		(*into)->UploadToGPU();
	};

	loadFunc("cube.msh"		 , &cubeMesh);
	loadFunc("sphere.msh"	 , &sphereMesh);
	loadFunc("Male1.msh"	 , &charMeshA);
	loadFunc("courier.msh"	 , &charMeshB);
	loadFunc("security.msh"	 , &enemyMesh);
	loadFunc("coin.msh"		 , &bonusMesh);
	loadFunc("capsule.msh"	 , &capsuleMesh);

	basicTex	= (OGLTexture*)TextureLoader::LoadAPITexture("checkerboard.png");
	basicShader = new OGLShader("GameTechVert.glsl", "GameTechFrag.glsl");

	InitCamera();
}

TutorialGame::~TutorialGame()	{
	delete cubeMesh;
	delete sphereMesh;
	delete charMeshA;
	delete charMeshB;
	delete enemyMesh;
	delete bonusMesh;

	delete basicTex;
	delete basicShader;

	delete physics;
	delete renderer;
	delete world;
}

void TutorialGame::ClearWorld() {

	pushers.clear();
	stateObjects.clear();
	world->ClearAndErase();
	physics->Clear();

	selectionObject = nullptr;
	lockedObject = nullptr;

	timer = new GameTimer();
}

void TutorialGame::UpdateGame(float dt) {
	if (!inSelectionMode) {
		world->GetMainCamera()->UpdateCamera(dt);
	}

	for (int i = 0; i < collectables.size(); i++) {
		if (!collectables[i]->IsActive()) {
			world->RemoveGameObject(collectables[i]);
			collectables.erase(collectables.begin() + i);
		}
	}


	UpdateKeys();

	if (player) {
		Debug::Print("Score: " + std::to_string(player->GetScore()), Vector2(5, 10));
		Debug::Print("Time: " + std::to_string((int)timer->GetTotalTimeSeconds()) + "s", Vector2(5, 15));

		switch (level) {
		case 1:
			//Win if player reaches goal
			if (player->GoalReached()) {
				won = true;
				finishTime = timer->GetTotalTimeSeconds();
				finishScore = player->GetScore();
			}

			//Lose if run out of time, or player killed
			if (timer->GetTotalTimeSeconds() > 60) {
				gameLost = true;
				finishTime = timer->GetTotalTimeSeconds();
				finishScore = player->GetScore();
			}
			break;
		case 2:
			//Win if player collects 3 or more coins
			if (player->GetScore() >= 30) {
				won = true;
				finishTime = timer->GetTotalTimeSeconds();
				finishScore = player->GetScore();
			}
			//Lose if touches enemy
			else if (player->ISGameLost()) {
				gameLost = true;
				finishTime = timer->GetTotalTimeSeconds();
				finishScore = player->GetScore();
			}
			break;
		}

			collectableInWorld = collectables.size() > 0 ? true : false;

		if (enemy) {
			if (coinSpawnTimer >= 10.0f) {
				collectables.emplace_back(SpawnCoin());
				coinSpawnTimer = 0;
			}
			else {
				coinSpawnTimer += dt;
			}

			Vector3 enemyPos = enemy->GetTransform().GetPosition() + Vector3(0, 10, 0);
			Ray ray = Ray(enemyPos, (player->GetTransform().GetPosition() - enemyPos).Normalised());
			RayCollision closestCollision;

			//If the enemy can see the player, chase it...
			if (world->Raycast(ray, closestCollision, true)) {
				if ((PlayerObj*)closestCollision.node == player) {
					enemy->SetTarget(player);
				}
				//...Otherwise, go after the most recently spawned collectable
				else {
					enemy->SetTarget(collectableInWorld ? collectables.back() : enemy);
				}
			}
			else {
				enemy->SetTarget(collectableInWorld ? collectables.back() : enemy);
			}

			if (enemy->GetTarget() != nullptr) {
				PathFind(enemy->GetTransform().GetPosition() + Vector3(10, 0, 10), enemy->GetTarget()->GetTransform().GetPosition() + Vector3(10, 0, 10));
				DebugDisplayPath();
			}
		}
	}


	if (drawColliders) {
		GameObjectIterator first;
		GameObjectIterator last;
		world->GetObjectIterators(first, last);
		for (auto i = first; i != last; i++) {
			DebugDrawCollider((*i)->GetBoundingVolume(), &(*i)->GetTransform());
		}
	}
	SelectObject();
	//MoveSelectedObject();
	physics->Update(dt);

	for (auto i : stateObjects) {
		((VerticalBlocker*)i)->Update(dt);
	}

	if (lockedObject != nullptr) {
		Vector3 objPos = lockedObject->GetTransform().GetPosition();
		Vector3 camPos = objPos + lockedOffset;

		Matrix4 temp = Matrix4::BuildViewMatrix(camPos, objPos, Vector3(0,1,0));

		Matrix4 modelMat = temp.Inverse();

		Quaternion q(modelMat);
		Vector3 angles = q.ToEuler(); //nearly there now!

		world->GetMainCamera()->SetPosition(camPos);
		world->GetMainCamera()->SetPitch(angles.x);
		world->GetMainCamera()->SetYaw(angles.y);

		//Debug::DrawAxisLines(lockedObject->GetTransform().GetMatrix(), 2.0f);
	}

	world->UpdateWorld(dt);
	renderer->Update(dt);

	Debug::FlushRenderables(dt);
	renderer->Render();
}

void TutorialGame::Menu(int option, float dt) {
	Vector4 selectedCol = Vector4(1, 0.5, 0.5, 1);

	Debug::Print("Marble Dome", Vector2(37, 30));
	Debug::Print("___________", Vector2(37, 35));

	switch (option) {
	case 0:
		Debug::Print("-Play Level 1", Vector2(35, 40), selectedCol);
		Debug::Print(" Play Level 2", Vector2(35, 50));
		Debug::Print(" Quit Game", Vector2(35, 60));
		break;
	case 1:
		Debug::Print(" Play Level 1", Vector2(35, 40));
		Debug::Print("-Play Level 2", Vector2(35, 50), selectedCol);
		Debug::Print(" Quit Game", Vector2(35, 60));
		break;
	case 2:
		Debug::Print(" Play Level 1", Vector2(35, 40));
		Debug::Print(" Play Level 2", Vector2(35, 50));
		Debug::Print("-Quit Game",	Vector2(35, 60), selectedCol);
		break;
	}

	Debug::FlushRenderables(dt);
	renderer->Update(dt);
	renderer->Render();

}

void TutorialGame::ShowScore(bool won, float dt) {
	Vector4 selectedCol = Vector4(1, 0.5, 0.5, 1);

	Debug::Print(won ? "     You Won     !" : "    You Lost!     ", Vector2(30, 30));
	Debug::Print("_____________________", Vector2(30, 35));
	Debug::Print("Score: " + std::to_string(finishScore), Vector2(30, 40));
	Debug::Print("Time Taken: " + std::to_string((int)finishTime), Vector2(30, 45));
	Debug::Print("Press [Esc] to return to the Main Menu", Vector2(12, 55));

	Debug::FlushRenderables(dt);
	renderer->Update(dt);
	renderer->Render();
}

void TutorialGame::UpdateKeys() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F1)) {
		selectionObject = nullptr;
		lockedObject	= nullptr;
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F2)) {
		InitCamera(); //F2 will reset the camera to a specific default place
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::G)) {
		useGravity = !useGravity; //Toggle gravity!
		physics->UseGravity(useGravity);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::P)) {
		drawColliders = !drawColliders; //Toggle Collider Drawing
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::L)) {
		for (Spring* s : pushers) {
			s->ToggleSpringCoil();

		}
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::Y)) {
		Vector3 camPos = world->GetMainCamera()->GetPosition();
		AddCubeToWorld(camPos + Vector3(5, 0, 0), Vector3(1, 1, 1), false);
	}
	//Running certain physics updates in a consistent order might cause some
	//bias in the calculations - the same objects might keep 'winning' the constraint
	//allowing the other one to stretch too much etc. Shuffling the order so that it
	//is random every frame can help reduce such bias.
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F9)) {
		world->ShuffleConstraints(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F10)) {
		world->ShuffleConstraints(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F7)) {
		world->ShuffleObjects(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F8)) {
		world->ShuffleObjects(false);
	}

	if (lockedObject) {
		LockedObjectMovement();
	}
	else {
		DebugObjectMovement();
	}

	if (controlBall && player) {
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
			player->GetPhysicsObject()->AddForce(Vector3(100, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
			player->GetPhysicsObject()->AddForce(Vector3(0, 0, -100));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
			player->GetPhysicsObject()->AddForce(Vector3(-100, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			player->GetPhysicsObject()->AddForce(Vector3(0, 0, 100));
		}
	}
}

void TutorialGame::LockedObjectMovement() {
	Matrix4 view		= world->GetMainCamera()->BuildViewMatrix();
	Matrix4 camWorld	= view.Inverse();

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	//forward is more tricky -  camera forward is 'into' the screen...
	//so we can take a guess, and use the cross of straight up, and
	//the right axis, to hopefully get a vector that's good enough!

	Vector3 fwdAxis = Vector3::Cross(Vector3(0, 1, 0), rightAxis);
	fwdAxis.y = 0.0f;
	fwdAxis.Normalise();

	Vector3 charForward  = lockedObject->GetTransform().GetOrientation() * Vector3(0, 0, 1);
	Vector3 charForward2 = lockedObject->GetTransform().GetOrientation() * Vector3(0, 0, 1);

	float force = 100.0f;

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
		lockedObject->GetPhysicsObject()->AddForce(-rightAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
		Vector3 worldPos = selectionObject->GetTransform().GetPosition();
		lockedObject->GetPhysicsObject()->AddForce(rightAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
		lockedObject->GetPhysicsObject()->AddForce(fwdAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
		lockedObject->GetPhysicsObject()->AddForce(-fwdAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NEXT)) {
		lockedObject->GetPhysicsObject()->AddForce(Vector3(0,-10,0));
	}
}

void TutorialGame::DebugObjectMovement() {
//If we've selected an object, we can manipulate it with some key presses
	if (inSelectionMode && selectionObject) {
		//Twist the selected object!
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(-10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM7)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM8)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM5)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -10, 0));
		}
	}

}

void TutorialGame::DebugDrawCollider(const CollisionVolume* c, Transform* worldTransform) {
	Vector4 col = Vector4(1, 0, 0, 1);
	switch (c->type) {
	case VolumeType::AABB: Debug::DrawCube(worldTransform->GetPosition(), ((AABBVolume*)c)->GetHalfDimensions(),col); break;
	case VolumeType::OBB: Debug::DrawCube(worldTransform->GetPosition(), ((AABBVolume*)c)->GetHalfDimensions(),Vector4(0,1,0,1), 0, worldTransform->GetOrientation()); break;
	case VolumeType::Sphere: Debug::DrawSphere(worldTransform->GetPosition(), ((SphereVolume*)c)->GetRadius(), col); break;
	case VolumeType::Capsule: DebugDrawCapsule((CapsuleVolume*)c, worldTransform); break;
	default: break;
	}
}
//Outlines the capsule, with spheres at the ends
void TutorialGame::DebugDrawCapsule(CapsuleVolume* a, Transform* worldTransform) {
	Vector4 col = Vector4(1, 0, 0, 1);
	//Add small spacings to the radius and halfheight so that they show up better through the mesh
	float radius = a->GetRadius();
	float halfHeight = a->GetHalfHeight();
	Vector3 pos = worldTransform->GetPosition();
	Quaternion rot = worldTransform->GetOrientation();
	Matrix3 transform = Matrix3(rot);

	Vector3 topPoint = pos + transform * Vector3(0, halfHeight - radius, 0);
	Vector3 bottomPoint = pos - transform * Vector3(0, halfHeight - radius, 0);

	//Draw sides of cylinder
	Debug::DrawLine(bottomPoint + transform * Vector3(radius, 0, 0), topPoint + transform * Vector3(radius, 0, 0), col);
	Debug::DrawLine(bottomPoint - transform * Vector3(radius, 0, 0), topPoint - transform * Vector3(radius, 0, 0), col);
	Debug::DrawLine(bottomPoint + transform * Vector3(0, 0, radius), topPoint + transform * Vector3(0, 0, radius), col);
	Debug::DrawLine(bottomPoint - transform * Vector3(0, 0, radius), topPoint - transform * Vector3(0, 0, radius), col);

	//Draw semicircles around top and bottom
	Debug::DrawSphere(topPoint, radius, col);
	Debug::DrawSphere(bottomPoint, radius, col);
}

void TutorialGame::InitCamera() {
	world->GetMainCamera()->SetNearPlane(0.1f);
	world->GetMainCamera()->SetFarPlane(500.0f);
	world->GetMainCamera()->SetPitch(-15.0f);
	world->GetMainCamera()->SetYaw(315.0f);
	world->GetMainCamera()->SetPosition(Vector3(-60, 40, 60));
	lockedObject = nullptr;
}

/*
A single function to add a large immoveable cube to the bottom of our world
*/
GameObject* TutorialGame::AddFloorToWorld(const Vector3& position, const Vector3& dims, const Quaternion& rotation, float elasticity, Vector4 col, string name) {
	GameObject* floor = new GameObject(name, Layer::StaticObjects);

	OBBVolume* volume	= new OBBVolume(dims);
	floor->GetTransform().SetOrientation(rotation);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform()
		.SetScale(dims * 2)
		.SetPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->GetRenderObject()->SetColour(col);
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->SetElasticity(elasticity);

	world->AddGameObject(floor);

	return floor;
}

VerticalBlocker* TutorialGame::AddVerticalBlockerToWorld(const Vector3& position, const Quaternion& rotation) {
	VerticalBlocker* s = new VerticalBlocker();
	s->SetLayer(Layer::AntiGravity);
	s->GetTransform().SetPosition(position).SetScale(Vector3(20, 40, 20)).SetOrientation(rotation);
	s->SetBoundingVolume(new CapsuleVolume(40, 10));

	s->SetRenderObject(new RenderObject(&s->GetTransform(), capsuleMesh, basicTex, basicShader));
	s->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));
	s->SetPhysicsObject(new PhysicsObject(&s->GetTransform(), s->GetBoundingVolume()));
	s->GetPhysicsObject()->SetElasticity(1.6);

	s->GetPhysicsObject()->SetInverseMass(0.1);
	s->GetPhysicsObject()->SetElasticity(1);
	world->AddGameObject(s);
	stateObjects.emplace_back(s);

	return s;
}

HorizontalBlocker* TutorialGame::AddHorizontalBlockerToWorld(const Vector3& position) {
	HorizontalBlocker* s = new HorizontalBlocker();
	Vector3 dims = Vector3(10, 10, 1);
	s->SetLayer(Layer::AntiGravity);
	s->GetTransform().SetPosition(position).SetScale(dims * 2);
	s->SetBoundingVolume((CollisionVolume*)new AABBVolume(Vector3(10,10, 1)));

	s->SetRenderObject(new RenderObject(&s->GetTransform(), cubeMesh, basicTex, basicShader));
	s->GetRenderObject()->SetColour(Vector4(0,1,0,1));
	s->SetPhysicsObject(new PhysicsObject(&s->GetTransform(), s->GetBoundingVolume()));
	s->GetPhysicsObject()->SetElasticity(1.6);

	s->GetPhysicsObject()->SetInverseMass(0.1);
	s->GetPhysicsObject()->SetElasticity(1);
	world->AddGameObject(s);
	stateObjects.emplace_back(s);

	return s;
}

/*

Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple' 
physics worlds. You'll probably need another function for the creation of OBB cubes too.

*/
GameObject* TutorialGame::AddSphereToWorld(const Vector3& position, float radius, float inverseMass, float elasticity, int layer) {
	GameObject* sphere = new GameObject();

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);

	sphere->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	sphere->GetPhysicsObject()->InitSphereInertia();
	sphere->GetPhysicsObject()->SetElasticity(elasticity);

	world->AddGameObject(sphere);

	return sphere;
}

GameObject* TutorialGame::AddSwitchToWorld(const Vector3& position) {
	GameObject* sw = new Switch();
	Vector3 sphereSize = Vector3(2, 2, 2);
	SphereVolume* volume = new SphereVolume(2);
	sw->SetBoundingVolume((CollisionVolume*)volume);
	sw->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);
	sw->SetRenderObject(new RenderObject(&sw->GetTransform(), sphereMesh, basicTex, basicShader));
	sw->GetRenderObject()->SetColour(Vector4(1, 0, 1, 1));
	sw->SetPhysicsObject(new PhysicsObject(&sw->GetTransform(), sw->GetBoundingVolume()));
	sw->GetPhysicsObject()->SetInverseMass(0);
	sw->GetPhysicsObject()->InitSphereInertia();
	sw->GetPhysicsObject()->SetElasticity(0);

	world->AddGameObject(sw);

	return sw;
}

GameObject* TutorialGame::AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, float inverseMass) {
	GameObject* capsule = new GameObject();

	CapsuleVolume* volume = new CapsuleVolume(halfHeight, radius);
	capsule->SetBoundingVolume((CollisionVolume*)volume);

	capsule->GetTransform()
		.SetScale(Vector3(radius* 2, halfHeight, radius * 2))
		.SetPosition(position);

	capsule->SetRenderObject(new RenderObject(&capsule->GetTransform(), capsuleMesh, basicTex, basicShader));
	capsule->SetPhysicsObject(new PhysicsObject(&capsule->GetTransform(), capsule->GetBoundingVolume()));

	capsule->GetPhysicsObject()->SetInverseMass(inverseMass);
	capsule->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(capsule);

	return capsule;

}

GameObject* TutorialGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, bool axisAligned,float inverseMass, int layer) {
	GameObject* cube = new GameObject("", layer);

	CollisionVolume* volume;
		volume = axisAligned ? 
			(CollisionVolume*) new AABBVolume(dimensions) : 
			(CollisionVolume*) new OBBVolume(dimensions);

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

void TutorialGame::InitLevel1() {

	ClearWorld();
	finishScore = 0;
	finishTime = 0;

	won = false;
	gameLost = false;
	controlBall = false;
	level = 1;

	//Floor
	AddFloorToWorld(Vector3(0, -3, 0), Vector3(20, 1, 20), Quaternion(1, 0, 0, 0), 0.7,Vector4(0.1,0.1,0.1,1));
	AddFloorToWorld(Vector3(38, -10.6, 0), Vector3(20, 1, 20), Quaternion(1, -0.2f, 0, 0),0.7, Vector4(0.1, 0.1, 0.1, 1));
	AddFloorToWorld(Vector3(75, -23, 0), Vector3(20, 1, 20), Quaternion(1, 0.4f, 0, 0),0.7, Vector4(0.1, 0.1, 0.1, 1));
	AddFloorToWorld(Vector3(47, -50.3, 0), Vector3(20, 1, 20), Quaternion(1, 0.4f, 0, 0),0.7, Vector4(0.1, 0.1, 0.1, 1));
	AddFloorToWorld(Vector3(9, -45, 70), Vector3(15, 1, 20), Quaternion(1, 0, 0, 0), 1.4f, Vector4(0.1, 1, 0.1, 1), "Bouncy Floor");
	AddFloorToWorld(Vector3(9, -45, 110), Vector3(15, 1, 20), Quaternion(1, 0, 0, 0), 1.4f, Vector4(0.1, 1, 0.1, 1), "Bouncy Floor");

	//Pushers
	AddPusher(Vector3(-6,0.1f,0), Vector3(1,2,4), Quaternion(1,0,0,0), true);
	AddPusher(Vector3(9, -70, 0), Vector3(20,1,20), Quaternion(1,1,0,0), true, 200, 7);
	AddPusher(Vector3(12, -35, -15), Vector3(20,5,1), Quaternion(1,0,1,0), true, 250, 10);

	//Blockers
	AddVerticalBlockerToWorld(Vector3(9, -15, 35), Quaternion::EulerAnglesToQuaternion(0,0,90));
	AddHorizontalBlockerToWorld(Vector3(20, 1, 13));

	//Coins
	AddBonusToWorld(Vector3(30, -2, 0), Quaternion::EulerAnglesToQuaternion(0,90,0));
	AddBonusToWorld(Vector3(50, -38, 0), Quaternion::EulerAnglesToQuaternion(0,90,0));

	AddPlayerToWorld(Vector3(0, 0, 0));

	AddGoal(Vector3(9, -40, 110));

	timer = new GameTimer();
}

void TutorialGame::InitLevel2() {

	ClearWorld();
	finishScore = 0;
	finishTime = 0;

	won = false;
	gameLost = false;
	controlBall = true;
	level = 2;

	AddFloorToWorld(Vector3(90, -4, 90), Vector3(90, 4, 90), Quaternion::EulerAnglesToQuaternion(0, 0, 0), 0.5f, Vector4(0.1,0.1,0.1,1));
	AddFloorToWorld(Vector3(100, 0, 0), Vector3(100, 10, 10), Quaternion::EulerAnglesToQuaternion(0, 0, 0));
	AddFloorToWorld(Vector3(100, 0, 180), Vector3(100, 10, 10), Quaternion::EulerAnglesToQuaternion(0, 0, 0));
	AddFloorToWorld(Vector3(0, 0, 100), Vector3(10, 10, 100), Quaternion::EulerAnglesToQuaternion(0, 0, 0));
	AddFloorToWorld(Vector3(180, 0, 100), Vector3(10, 10, 100), Quaternion::EulerAnglesToQuaternion(0, 0, 0));
	AddFloorToWorld(Vector3(90, 0, 160), Vector3(20, 10, 10), Quaternion::EulerAnglesToQuaternion(0, 0, 0));
	AddFloorToWorld(Vector3(90, 0, 120), Vector3(60, 10, 10), Quaternion::EulerAnglesToQuaternion(0, 0, 0));
	AddFloorToWorld(Vector3(40, 0, 70), Vector3(10, 10, 40), Quaternion::EulerAnglesToQuaternion(0, 0, 0));
	AddFloorToWorld(Vector3(140, 0, 70), Vector3(10, 10, 40), Quaternion::EulerAnglesToQuaternion(0, 0, 0));
	AddFloorToWorld(Vector3(90, 0, 60), Vector3(20, 10, 10), Quaternion::EulerAnglesToQuaternion(0, 0, 0));
	AddFloorToWorld(Vector3(120, 0, 80), Vector3(10, 10, 10), Quaternion::EulerAnglesToQuaternion(0, 0, 0));

	AddVerticalBlockerToWorld(Vector3(90, -10, 140), Quaternion::EulerAnglesToQuaternion(0,0,0));

	AddPlayerToWorld(Vector3(20, 20, 20));
	AddEnemyToWorld(Vector3(160, 4, 160));


	timer = new GameTimer();
}

void TutorialGame::AddPusher(Vector3 pos, Vector3 pusherDims,  Quaternion rot, bool startCoiled, float springForce, float length) {
	GameObject* s1 = AddSwitchToWorld(pos);
	GameObject* s2 = AddCubeToWorld(pos + rot * Vector3(5,0,0), pusherDims, true, 1, Layer::Clickable|Layer::AntiGravity);
	s2->GetRenderObject()->SetColour(Vector4(1,0.5,0.5,1));
	Spring* spr = new Spring(s1, s2, length, springForce, startCoiled);
	((Switch*)s1)->SetSpring(spr);
	world->AddConstraint(spr);
	pushers.emplace_back(spr);
}

GameObject* TutorialGame::AddPlayerToWorld(const Vector3& position) {
	float radius = 3.0f;
	PlayerObj* sphere = new PlayerObj();

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);

	sphere->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->GetRenderObject()->SetColour(Vector4(0, 0, 1, 1));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(1.0f);
	sphere->GetPhysicsObject()->InitSphereInertia();
	sphere->GetPhysicsObject()->SetElasticity(0.8f);

	world->AddGameObject(sphere);
	player = sphere;

	return sphere;
}

GameObject* TutorialGame::AddEnemyToWorld(const Vector3& position) {
	float meshSize		= 3.0f;
	float inverseMass	= 0.5f;

	Enemy* enemy = new Enemy(pathNodes, player);

	SphereVolume* volume = new SphereVolume(3);
	enemy->SetBoundingVolume((CollisionVolume*)volume);

	enemy->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	enemy->SetRenderObject(new RenderObject(&enemy->GetTransform(), enemyMesh, nullptr, basicShader));
	enemy->GetRenderObject()->SetColour(Vector4(1,0,0,1));
	enemy->SetPhysicsObject(new PhysicsObject(&enemy->GetTransform(), enemy->GetBoundingVolume()));

	enemy->GetPhysicsObject()->SetInverseMass(inverseMass);
	enemy->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(enemy);
	stateObjects.emplace_back(enemy);
	TutorialGame::enemy = enemy;

	return enemy;
}

GameObject* TutorialGame::AddBonusToWorld(const Vector3& position, const Quaternion& rot) {
	Coin* coin = new Coin();

	SphereVolume* volume = new SphereVolume(0.25f);
	coin->SetBoundingVolume((CollisionVolume*)volume);
	coin->GetTransform()
		.SetScale(Vector3(0.25, 0.25, 0.25))
		.SetPosition(position)
		.SetOrientation(rot);

	coin->SetRenderObject(new RenderObject(&coin->GetTransform(), bonusMesh, nullptr, basicShader));
	coin->GetRenderObject()->SetColour(Vector4(1, 1, 0, 1));
	coin->SetPhysicsObject(new PhysicsObject(&coin->GetTransform(), coin->GetBoundingVolume()));

	coin->GetPhysicsObject()->SetInverseMass(1.0f);
	coin->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(coin);

	return coin;
}

Goal* TutorialGame::AddGoal(const Vector3& position) {
	//Add large goal collision volume, that looks like a flagpole
	Goal* goal = new Goal();
	AABBVolume* volume = new AABBVolume(Vector3(5,5,5));
	goal->SetBoundingVolume((CollisionVolume*)volume);
	goal->GetTransform()
		.SetScale(Vector3(1, 10, 1))
		.SetPosition(position);
	goal->SetRenderObject(new RenderObject(&goal->GetTransform(), cubeMesh, nullptr, basicShader));
	goal->GetRenderObject()->SetColour(Vector4(0.5, 0.2f, 0, 1));
	goal->SetPhysicsObject(new PhysicsObject(&goal->GetTransform(), goal->GetBoundingVolume()));

	goal->GetPhysicsObject()->SetInverseMass(0);
	goal->GetPhysicsObject()->InitSphereInertia();
	world->AddGameObject(goal);

	//Add small 'flag' at the top, that doesn't interact with anything

	GameObject* flag = new GameObject("Flag", Layer::IgnoreAllCollisions);
	AABBVolume* flagVolume = new AABBVolume(Vector3(0.05f, 1, 2));
	flag->SetBoundingVolume((CollisionVolume*)flagVolume);
	flag->GetTransform()
		.SetScale(Vector3(0.1f, 2, 4))
		.SetPosition(position + Vector3(0, 3.5f,-1.5f));
	flag->SetRenderObject(new RenderObject(&flag->GetTransform(), cubeMesh, nullptr, basicShader));
	flag->GetRenderObject()->SetColour(Vector4(1, 0, 0, 1));
	flag->SetPhysicsObject(new PhysicsObject(&flag->GetTransform(), flag->GetBoundingVolume()));

	flag->GetPhysicsObject()->SetInverseMass(0);
	flag->GetPhysicsObject()->InitSphereInertia();
	world->AddGameObject(flag);

	return goal;
};

/*

Every frame, this code will let you perform a raycast, to see if there's an object
underneath the cursor, and if so 'select it' into a pointer, so that it can be 
manipulated later. Pressing Q will let you toggle between this behaviour and instead
letting you move the camera around. 

*/
bool TutorialGame::SelectObject() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::Q)) {
		inSelectionMode = !inSelectionMode;
		if (inSelectionMode) {
			Window::GetWindow()->ShowOSPointer(true);
			Window::GetWindow()->LockMouseToWindow(false);
		}
		else {
			Window::GetWindow()->ShowOSPointer(false);
			Window::GetWindow()->LockMouseToWindow(true);
		}
	}
	if (inSelectionMode) {
		//renderer->DrawString("Press Q to change to camera mode!", Vector2(5, 85));

		if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::LEFT)) {
			if (selectionObject) {	//set colour to deselected;
				selectionObject = nullptr;
				lockedObject	= nullptr;
			}

			Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

			RayCollision closestCollision;
			if (world->Raycast(ray, closestCollision, true)) {
				selectionObject = (GameObject*)closestCollision.node;
				selectionObject->OnSelect();
				return true;
			}
			else {
				return false;
			}
		}
	}
	else {
		renderer->DrawString("Press Q to change to select mode!", Vector2(5, 85));
		renderer->DrawString("Press L to toggle the pushers,", Vector2(5, 90));
		renderer->DrawString("or click on the purple buttons!", Vector2(5, 95));
	}

	if(selectionObject){
		renderer->DrawString("Name: " + selectionObject->GetName(), Vector2(1, 75));
		Transform trans = selectionObject->GetTransform();
		Vector3 pos = trans.GetPosition();
		renderer->DrawString("Position: " + std::to_string(int(pos.x)) + ", " +
			std::to_string((int)pos.y) + ", " + std::to_string((int)pos.z), Vector2(1, 80));

		Vector3 rot = trans.GetOrientation().ToEuler();
		renderer->DrawString("Rotation: " + std::to_string(int(rot.x)) + ", " +
			std::to_string((int)rot.y) + ", " + std::to_string((int)rot.z), Vector2(1, 85));

		Debug::DrawLine(pos, pos + selectionObject->GetPhysicsObject()->GetLinearVelocity(), Vector4(1,0,1,1));
		DebugDrawCollider(selectionObject->GetBoundingVolume(), &trans);
	}


	return false;
}

/*
If an object has been clicked, it can be pushed with the right mouse button, by an amount
determined by the scroll wheel. In the first tutorial this won't do anything, as we haven't
added linear motion into our physics system. After the second tutorial, objects will move in a straight
line - after the third, they'll be able to twist under torque aswell.
*/
void TutorialGame::MoveSelectedObject() {
	forceMagnitude += Window::GetMouse()->GetWheelMovement() * 100.0f;

	if (!selectionObject)
		return;

	if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::RIGHT)) {
		Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());
		RayCollision closestCollision;
		if (world->Raycast(ray, closestCollision, true)) {
			if (closestCollision.node == selectionObject) {
				selectionObject->GetPhysicsObject()->AddForceAtPosition(ray.GetDirection() * forceMagnitude, closestCollision.collidedAt);
			}
		}
	}
}

void TutorialGame::PathFind(Vector3 from, Vector3 to) {
	NavigationGrid grid("TestGrid1.txt");
	pathNodes.clear();
	NavigationPath outPath;
	bool found = grid.FindPath(from, to, outPath);
	Vector3 pos;
	while (outPath.PopWaypoint(pos)) {
		pathNodes.push_back(pos);

	}
}

GameObject* TutorialGame::SpawnCoin() {
	return AddBonusToWorld(validSpawnPositions[rand()%validSpawnPositions.size()], Quaternion(0,0,0,0));
}

void TutorialGame::DebugDisplayPath() {
	for (int i = 1; i < pathNodes.size(); ++i) {
		Vector3 a = pathNodes[i - 1];
		Vector3 b = pathNodes[i];

		Debug::DrawLine(a, b, Vector4(0, 1, 0, 1));

	}
}