#include "TutorialGame.h"
#include "../CSC8503Common/GameWorld.h"
#include "../../Plugins/OpenGLRendering/OGLMesh.h"
#include "../../Plugins/OpenGLRendering/OGLShader.h"
#include "../../Plugins/OpenGLRendering/OGLTexture.h"
#include "../../Common/TextureLoader.h"
#include"../CSC8503Common/PositionConstraint.h"

using namespace NCL;
using namespace CSC8503;

TutorialGame::TutorialGame()	{
	world		= new GameWorld();
	renderer	= new GameTechRenderer(*world);
	physics		= new PhysicsSystem(*world);

	forceMagnitude	= 10.0f;
	useGravity		= false;
	inSelectionMode = false;

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
	InitWorld();
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

void TutorialGame::UpdateGame(float dt) {
	if (!inSelectionMode) {
		world->GetMainCamera()->UpdateCamera(dt);
	}

	UpdateKeys();

	if (useGravity) {
		Debug::Print("(G)ravity on", Vector2(5, 95));
	}
	else {
		Debug::Print("(G)ravity off", Vector2(5, 95));
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
	MoveSelectedObject();
	physics->Update(dt);

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

void TutorialGame::UpdateKeys() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F1)) {
		InitWorld(); //We can reset the simulation at any time with F1
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
	switch (c->type) {
	case VolumeType::AABB: DebugDrawAABB((AABBVolume*)c, worldTransform); break;
	case VolumeType::Sphere: DebugDrawSphere((SphereVolume*)c, worldTransform); break;
	case VolumeType::Capsule: DebugDrawCapsule((CapsuleVolume*)c, worldTransform); break;
	default: break;
	}
}
//Draws a line along each edge of the AABB
void TutorialGame::DebugDrawAABB(AABBVolume* a, Transform* worldTransform) {
	//Add small spacing to the halfsizes, so they show above the mesh
	Vector3 halfSizes = a->GetHalfDimensions() + Vector3(0.01f, 0.01f, 0.01f);
	Vector3 position = worldTransform->GetPosition();
	
	Vector4 col = Vector4(1, 0, 0, 1);

	//Draw all of the lines making up the AABB (no need for rotation)
	Debug::DrawLine(position + halfSizes, position + (Vector3(-halfSizes.x, halfSizes.y, halfSizes.z)), col);
	Debug::DrawLine(position + halfSizes, position + (Vector3(halfSizes.x, -halfSizes.y, halfSizes.z)), col);
	Debug::DrawLine(position + halfSizes, position + (Vector3(halfSizes.x, halfSizes.y, -halfSizes.z)), col);
	Debug::DrawLine(position - halfSizes, position + (Vector3(halfSizes.x, -halfSizes.y, -halfSizes.z)), col);
	Debug::DrawLine(position - halfSizes, position + (Vector3(-halfSizes.x, halfSizes.y, -halfSizes.z)), col);
	Debug::DrawLine(position - halfSizes, position + (Vector3(-halfSizes.x, -halfSizes.y, halfSizes.z)), col);
	Debug::DrawLine(position + (Vector3(-halfSizes.x, halfSizes.y, halfSizes.z)), position + (Vector3(-halfSizes.x, halfSizes.y, -halfSizes.z)), col);
	Debug::DrawLine(position + (Vector3(halfSizes.x, -halfSizes.y, halfSizes.z)), position + (Vector3(halfSizes.x, -halfSizes.y, -halfSizes.z)), col);
	Debug::DrawLine(position + (Vector3(halfSizes.x, -halfSizes.y, -halfSizes.z)), position + (Vector3(halfSizes.x, halfSizes.y, -halfSizes.z)), col);
	Debug::DrawLine(position + (Vector3(halfSizes.x, -halfSizes.y, halfSizes.z)), position + (Vector3(-halfSizes.x, -halfSizes.y, halfSizes.z)), col);
	Debug::DrawLine(position + (Vector3(-halfSizes.x, -halfSizes.y, halfSizes.z)), position + (Vector3(-halfSizes.x, halfSizes.y, halfSizes.z)), col);
	Debug::DrawLine(position + (Vector3(-halfSizes.x, halfSizes.y, -halfSizes.z)), position + (Vector3(halfSizes.x, halfSizes.y, -halfSizes.z)), col);
}
//Draws three circles, one along each sphere axis
void TutorialGame::DebugDrawSphere(SphereVolume* a, Transform* worldTransform) {
	float step = 0.5f;
	float radius = a->GetRadius() + 0.05f;
	//To allow lines to be on the surface of the sphere at lower 'resolutions'
	Vector4 col = Vector4(1, 0, 0, 1);
	Vector3 pos = worldTransform->GetPosition();
	Vector3 prevPoint = pos + Vector3(radius, 0 , 0);
	//Draw One along x axis...
	for (float angle = 0.0f; angle < 6.28f; angle += step) {
		Vector3 pointOnSphere = Vector3(cos(angle) * radius + pos.x, sin(angle) * radius + pos.y, pos.z);
		Debug::DrawLine(prevPoint, pointOnSphere, col);
		prevPoint = pointOnSphere;
	}
	prevPoint = pos + Vector3(0, 0, radius);
	//And one along the z axis...
	for (float angle = 0.0f; angle < 6.28f; angle += step) {
		Vector3 pointOnSphere = Vector3(pos.x, sin(angle) * radius + pos.y, cos(angle) * radius + pos.z);
		Debug::DrawLine(prevPoint, pointOnSphere, col);
		prevPoint = pointOnSphere;
	}
	prevPoint = pos + Vector3(radius, 0, 0);
	//And one along the z axis...
	for (float angle = 0.0f; angle < 6.28f; angle += step) {
		Vector3 pointOnSphere = Vector3(cos(angle) * radius + pos.x, pos.y, sin(angle) * radius + pos.z);
		Debug::DrawLine(prevPoint, pointOnSphere, col);
		prevPoint = pointOnSphere;
	}
}
//Outlines the capsule, with spheres at the ends
void TutorialGame::DebugDrawCapsule(CapsuleVolume* a, Transform* worldTransform) {
	Vector4 col = Vector4(1, 0, 0, 1);
	float step = 0.6f;
	//Add small spacings to the radius and halfheight so that they show up better through the mesh
	float radius = a->GetRadius() + 0.01f;
	float halfHeight = a->GetHalfHeight() + 0.01f;
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
	Vector3 prevPoint = topPoint + Vector3(-radius, 0, 0);
	for (float angle = 0.0f; angle < 6.28f; angle += step) {
		Vector3 pointOnSphere = Vector3(cos(angle) * radius + topPoint.x, sin(angle) * radius + topPoint.y, topPoint.z);
		Debug::DrawLine(prevPoint, pointOnSphere, col);
		prevPoint = pointOnSphere;
	}
	prevPoint = topPoint + Vector3(0, 0, -radius);
	for (float angle = 0.0f; angle < 6.28f; angle += step) {
		Vector3 pointOnSphere = Vector3(topPoint.x, sin(angle) * radius + topPoint.y, cos(angle) * radius + topPoint.z);
		Debug::DrawLine(prevPoint, pointOnSphere, col);
		prevPoint = pointOnSphere;
	}
	prevPoint = bottomPoint + Vector3(-radius, 0, 0);
	for (float angle = 0.0f; angle < 6.28f; angle += step) {
		Vector3 pointOnSphere = Vector3(cos(angle) * radius + bottomPoint.x, sin(angle) * radius + bottomPoint.y, bottomPoint.z);
		Debug::DrawLine(prevPoint, pointOnSphere, col);
		prevPoint = pointOnSphere;
	}
	prevPoint = bottomPoint + Vector3(0, 0, -radius);
	for (float angle = 0.0f; angle < 6.28f; angle += step) {
		Vector3 pointOnSphere = Vector3(bottomPoint.x, sin(angle) * radius + bottomPoint.y, cos(angle) * radius + bottomPoint.z);
		Debug::DrawLine(prevPoint, pointOnSphere, col);
		prevPoint = pointOnSphere;
	}
}

void TutorialGame::InitCamera() {
	world->GetMainCamera()->SetNearPlane(0.1f);
	world->GetMainCamera()->SetFarPlane(500.0f);
	world->GetMainCamera()->SetPitch(-15.0f);
	world->GetMainCamera()->SetYaw(315.0f);
	world->GetMainCamera()->SetPosition(Vector3(-60, 40, 60));
	lockedObject = nullptr;
}

void TutorialGame::InitWorld() {
	world->ClearAndErase();
	physics->Clear();

	InitMixedGridWorld(5, 5, 3.5f, 3.5f);
	//InitElasticitySphereGrid(5, 5, 1.0f);
	//InitGameExamples();
	//InitColliderTest();
	//InitDefaultFloor();
	//BridgeConstraintTest();
}

void TutorialGame::BridgeConstraintTest() {
	Vector3 cubeSize = Vector3(5, 5, 5);
	float invCubeMass = 10;
	int numLinks = 10;
	float maxDistance = 30;
	float cubeDistance = 20;

	Vector3 startPos = Vector3(50, 50, 50);
	GameObject* start = AddCubeToWorld(startPos + Vector3(0, 0, 0), cubeSize, 0);
	GameObject* end = AddCubeToWorld(startPos + Vector3((numLinks + 2) * cubeDistance, 0, 0), cubeSize, 0);
	GameObject* previous = start;

	for (int i = 0; i < numLinks; i++) {
		GameObject* block = AddCubeToWorld(startPos + Vector3((i + 1) * cubeDistance, 0, 0), cubeSize, invCubeMass);
		PositionConstraint* posConstraint = new PositionConstraint(previous, block, maxDistance);
		world->AddConstraint(posConstraint);
		previous = block;
	}
	PositionConstraint* posConstraint = new PositionConstraint(previous, end, maxDistance);
	world->AddConstraint(posConstraint);

}

/*

A single function to add a large immoveable cube to the bottom of our world

*/
GameObject* TutorialGame::AddFloorToWorld(const Vector3& position) {
	GameObject* floor = new GameObject();

	Vector3 floorSize	= Vector3(100, 2, 100);
	AABBVolume* volume	= new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform()
		.SetScale(floorSize * 2)
		.SetPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(floor);

	return floor;
}

/*

Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple' 
physics worlds. You'll probably need another function for the creation of OBB cubes too.

*/
GameObject* TutorialGame::AddSphereToWorld(const Vector3& position, float radius, float inverseMass, float elasticity) {
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

GameObject* TutorialGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	GameObject* cube = new GameObject();

	AABBVolume* volume = new AABBVolume(dimensions);

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

void TutorialGame::InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddSphereToWorld(position, radius, 1.0f);
		}
	}
	AddFloorToWorld(Vector3(0, -2, 0));
}

void TutorialGame::InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing) {
	float sphereRadius = 1.0f;
	Vector3 cubeDims = Vector3(1, 1, 1);
	float capsuleHH = 2;
	float capsuleRadius = 1.0f;

	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);

			if (rand() % 2) {
				AddCubeToWorld(position, cubeDims);
			}
			else if (rand() % 3) {
				AddCapsuleToWorld(position, capsuleHH, capsuleRadius);
			}
			else {
				AddSphereToWorld(position, sphereRadius);
			}
		}
	}
}

void TutorialGame::InitColliderTest() {
	AddCapsuleToWorld(Vector3(10, 10, 10), 2, 1);
	AddSphereToWorld(Vector3(15,10,10), 1);
}

void TutorialGame::InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {
	for (int x = 1; x < numCols+1; ++x) {
		for (int z = 1; z < numRows+1; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddCubeToWorld(position, cubeDims, 1.0f);
		}
	}
}

void TutorialGame::InitElasticitySphereGrid(int numRows, float rowSpacing, float sphereRadii) {
	//For testing 'bounciness' of the elasticity of spheres
	for (int x = 1; x < numRows + 1; ++x) {
			Vector3 position = Vector3(x * rowSpacing, 20.0f, 0);
			AddSphereToWorld(position, sphereRadii, 1.0f, ((float)x / numRows));
	}
}

void TutorialGame::InitDefaultFloor() {
	AddFloorToWorld(Vector3(0, -2, 0));
}

void TutorialGame::InitGameExamples() {
	AddPlayerToWorld(Vector3(0, 5, 0));
	AddEnemyToWorld(Vector3(5, 5, 0));
	AddBonusToWorld(Vector3(10, 5, 0));
}

GameObject* TutorialGame::AddPlayerToWorld(const Vector3& position) {
	float meshSize = 3.0f;
	float inverseMass = 0.5f;

	GameObject* character = new GameObject();

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.85f, 0.3f) * meshSize);

	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	if (rand() % 2) {
		character->SetRenderObject(new RenderObject(&character->GetTransform(), charMeshA, nullptr, basicShader));
	}
	else {
		character->SetRenderObject(new RenderObject(&character->GetTransform(), charMeshB, nullptr, basicShader));
	}
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(character);

	//lockedObject = character;

	return character;
}

GameObject* TutorialGame::AddEnemyToWorld(const Vector3& position) {
	float meshSize		= 3.0f;
	float inverseMass	= 0.5f;

	GameObject* character = new GameObject();

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.9f, 0.3f) * meshSize);
	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), enemyMesh, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(character);

	return character;
}

GameObject* TutorialGame::AddBonusToWorld(const Vector3& position) {
	GameObject* apple = new GameObject();

	SphereVolume* volume = new SphereVolume(0.25f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform()
		.SetScale(Vector3(0.25, 0.25, 0.25))
		.SetPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), bonusMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(apple);

	return apple;
}

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
		renderer->DrawString("Press Q to change to camera mode!", Vector2(5, 85));

		if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::LEFT)) {
			if (selectionObject) {	//set colour to deselected;
				selectionObject->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
				selectionObject = nullptr;
				lockedObject	= nullptr;
			}

			Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

			RayCollision closestCollision;
			if (world->Raycast(ray, closestCollision, true)) {
				selectionObject = (GameObject*)closestCollision.node;
				selectionObject->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));
				return true;
			}
			else {
				return false;
			}
		}
	}
	else {
		renderer->DrawString("Press Q to change to select mode!", Vector2(5, 85));
	}

	if (lockedObject) {
		renderer->DrawString("Press L to unlock object!", Vector2(5, 80));
	}

	else if(selectionObject){
		renderer->DrawString("Press L to lock selected object object!", Vector2(5, 80));
	}

	if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::L)) {
		if (selectionObject) {
			if (lockedObject == selectionObject) {
				lockedObject = nullptr;
			}
			else {
				lockedObject = selectionObject;
			}
		}

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
	renderer->DrawString("Click Force: " + std::to_string(forceMagnitude), Vector2(10, 20));
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