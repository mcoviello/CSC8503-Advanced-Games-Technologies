#pragma once
#include "GameTechRenderer.h"
#include "../CSC8503Common/PhysicsSystem.h"

namespace NCL {
	namespace CSC8503 {
		class Spring;
		class VerticalBlocker;
		class PlayerObj;
		class Goal;
		class TutorialGame		{
		public:
			TutorialGame();
			~TutorialGame();

			virtual void UpdateGame(float dt);

			void InitLevel1();
			void InitLevel2();
			void Menu(int option, float dt);
			void ShowScore(float dt);

			bool exitGame = false;
			bool goalReached;

		protected:
			void InitialiseAssets();

			void InitCamera();
			void UpdateKeys();

			void InitWorld();

			void BridgeConstraintTest();
	
			bool SelectObject();
			void MoveSelectedObject();
			void DebugObjectMovement();
			void LockedObjectMovement();
			void DebugDrawCollider(const CollisionVolume* c, Transform* worldTransform);
			void DebugDrawCapsule(CapsuleVolume* a, Transform* worldTransform);

			GameObject* AddFloorToWorld(const Vector3& position, const Vector3& dims, const Quaternion& rotation, float elasticity = 0.5f);
			GameObject* AddSphereToWorld(const Vector3& position, float radius, float inverseMass = 10.0f, float elasticity = 0.66f, Layer layer = Layer::Other);
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, bool axisAligned,float inverseMass = 10.0f, Layer layer = Layer::Other);
			GameObject* AddSwitchToWorld(const Vector3& position);
			void AddPusher(Vector3 pos, Vector3 pusherDims, Quaternion rot, bool startCoiled, float springForce = 200.0f, float length = 5.0f);
			VerticalBlocker* AddVerticalBlockerToWorld(const Vector3& position, const Quaternion& rotation);
			Goal* AddGoal(const Vector3& position);

			GameObject* AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, float inverseMass = 10.0f);

			GameObject* AddPlayerToWorld(const Vector3& position);
			GameObject* AddEnemyToWorld(const Vector3& position);
			GameObject* AddBonusToWorld(const Vector3& position);

			GameTechRenderer*	renderer;
			PhysicsSystem*		physics;
			GameWorld*			world;

			std::vector<Spring*> pushers;
			std::vector<GameObject*> stateObjects;

			bool useGravity;
			bool inSelectionMode;
			bool drawColliders;
			bool gameStarted = false;

			float		forceMagnitude;

			GameObject* selectionObject = nullptr;

			OGLMesh*	capsuleMesh = nullptr;
			OGLMesh*	cubeMesh	= nullptr;
			OGLMesh*	sphereMesh	= nullptr;
			OGLTexture* basicTex	= nullptr;
			OGLShader*	basicShader = nullptr;

			//Coursework Meshes
			OGLMesh*	charMeshA	= nullptr;
			OGLMesh*	charMeshB	= nullptr;
			OGLMesh*	enemyMesh	= nullptr;
			OGLMesh*	bonusMesh	= nullptr;

			//Coursework Additional functionality	
			GameObject* lockedObject	= nullptr;
			Vector3 lockedOffset		= Vector3(0, 14, 20);
			void LockCameraToObject(GameObject* o) {
				lockedObject = o;
			}

			GameTimer* timer;
			float finishTime;

			PlayerObj* player;

		};
	}
}