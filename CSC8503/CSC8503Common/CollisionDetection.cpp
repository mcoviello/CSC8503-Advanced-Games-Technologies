#include "CollisionDetection.h"
#include "CollisionVolume.h"
#include "AABBVolume.h"
#include "OBBVolume.h"
#include "SphereVolume.h"
#include "../../Common/Vector2.h"
#include "../../Common/Window.h"
#include "../../Common/Maths.h"
#include "Debug.h"

#include <list>

using namespace NCL;

bool CollisionDetection::RayPlaneIntersection(const Ray&r, const Plane&p, RayCollision& collisions) {
	float ln = Vector3::Dot(p.GetNormal(), r.GetDirection());

	if (ln == 0.0f) {
		return false; //direction vectors are perpendicular!
	}
	
	Vector3 planePoint = p.GetPointOnPlane();

	Vector3 pointDir = planePoint - r.GetPosition();

	float d = Vector3::Dot(pointDir, p.GetNormal()) / ln;

	collisions.collidedAt = r.GetPosition() + (r.GetDirection() * d);

	return true;
}

bool CollisionDetection::RayIntersection(const Ray& r,GameObject& object, RayCollision& collision) {
	bool hasCollided = false;

	const Transform& worldTransform = object.GetTransform();
	const CollisionVolume* volume	= object.GetBoundingVolume();

	if (!volume) {
		return false;
	}

	switch (volume->type) {
		case VolumeType::AABB:		hasCollided = RayAABBIntersection(r, worldTransform, (const AABBVolume&)*volume	, collision); break;
		case VolumeType::OBB:		hasCollided = RayOBBIntersection(r, worldTransform, (const OBBVolume&)*volume	, collision); break;
		case VolumeType::Sphere:	hasCollided = RaySphereIntersection(r, worldTransform, (const SphereVolume&)*volume	, collision); break;
		case VolumeType::Capsule:	hasCollided = RayCapsuleIntersection(r, worldTransform, (const CapsuleVolume&)*volume, collision); break;
	}

	return hasCollided;
}

bool CollisionDetection::RayBoxIntersection(const Ray&r, const Vector3& boxPos, const Vector3& boxSize, RayCollision& collision) {
	Vector3 boxMin = boxPos - boxSize;
	Vector3 boxMax = boxPos + boxSize;

	Vector3 rayPos = r.GetPosition();
	Vector3 rayDir = r.GetDirection();

	Vector3 tVals(-1, -1, -1);

	for (int i = 0; i < 3; i++) { //Get best 3 intersections
		if (rayDir[i] > 0) {
			tVals[i] = (boxMin[i] - rayPos[i]) / rayDir[i];
		}
		else if (rayDir[i] < 0) {
			tVals[i] = (boxMax[i] - rayPos[i]) / rayDir[i];
		}
	}
	float bestT = tVals.GetMaxElement();
	if (bestT < 0.0f) {
		return false; //No backwards rays!
	}

	Vector3 intersection = rayPos + (rayDir * bestT);
	const float epsilon = 0.0001f; //float leeway
	for (int i = 0; i < 3; i++) {
		//Intersection doesn't touch box...
		if (intersection[i] + epsilon < boxMin[i] ||
			intersection[i] - epsilon > boxMax[i]) {
			return false;
		}
	}
	collision.collidedAt = intersection;
	collision.rayDistance = bestT;
	return true;
}

bool CollisionDetection::RayAABBIntersection(const Ray&r, const Transform& worldTransform, const AABBVolume& volume, RayCollision& collision) {
	Vector3 boxPos = worldTransform.GetPosition();
	Vector3 boxSize = volume.GetHalfDimensions();
	return RayBoxIntersection(r, boxPos, boxSize, collision);
}

bool CollisionDetection::RayOBBIntersection(const Ray&r, const Transform& worldTransform, const OBBVolume& volume, RayCollision& collision) {
	Quaternion orientation = worldTransform.GetOrientation();
	Vector3 position = worldTransform.GetPosition();

	Matrix3 transform = Matrix3(orientation);
	Matrix3 invTransform = Matrix3(orientation.Conjugate());

	Vector3 localRayPos = r.GetPosition() - position;

	Ray tempRay(invTransform * localRayPos, invTransform * r.GetDirection());

	bool collided = RayBoxIntersection(tempRay, Vector3(), volume.GetHalfDimensions(), collision);

	if (collided) {
		collision.collidedAt = transform * collision.collidedAt + position;
	}

	return collided;
}

bool CollisionDetection::RayCapsuleIntersection(const Ray& r, const Transform& worldTransform, const CapsuleVolume& volume, RayCollision& collision) {
	Quaternion orientation = worldTransform.GetOrientation();
	Vector3 position = worldTransform.GetPosition();
	Matrix3 transform = Matrix3(orientation);

	Vector3 originToRayVec = position - r.GetPosition();
	Vector3 capsuleUpVec = transform * Vector3(0, 1, 0);
	Vector3 orthVec = Vector3::Cross(capsuleUpVec, originToRayVec);
	Plane p = Plane::PlaneFromTri(position, position + capsuleUpVec, position + orthVec);

	Vector3 topSpherePos = position + capsuleUpVec * (volume.GetHalfHeight() - volume.GetRadius());
	Vector3 bottomSpherePos = position - capsuleUpVec * (volume.GetHalfHeight() - volume.GetRadius());

	RayCollision rayC;
	RayPlaneIntersection(r, p, rayC);
	Vector3 planeCollision = rayC.collidedAt;

	bool collided = false;

	Vector3 bestSphereCheck;
	if (Vector3::Dot(topSpherePos - position, topSpherePos - planeCollision) < 0) {
		collided = Vector3::Distance(topSpherePos, planeCollision) < volume.GetRadius();
		bestSphereCheck = topSpherePos;
	}
	else if (Vector3::Dot(bottomSpherePos - position, bottomSpherePos - planeCollision) < 0) {
		collided = Vector3::Distance(bottomSpherePos, planeCollision) < volume.GetRadius();
		bestSphereCheck = bottomSpherePos;
	}
	else { //Collision lies on cylinder part of capsule
		Vector3 centralPoint = position + capsuleUpVec * (Vector3::Dot(planeCollision - position, capsuleUpVec));
		collided = Vector3::Distance(centralPoint, planeCollision) < volume.GetRadius();
		bestSphereCheck = centralPoint;
	}

	if (collided) {
		//Collision, do a ray sphere intersection test to calculate collision details
		return RaySphereIntersection(r, Transform().SetPosition(bestSphereCheck), SphereVolume(volume.GetRadius()), collision);
	}

	return false;
}

bool CollisionDetection::RaySphereIntersection(const Ray&r, const Transform& worldTransform, const SphereVolume& volume, RayCollision& collision) {
	Vector3 spherePos = worldTransform.GetPosition();
	float sphereRadius = volume.GetRadius();

	//Dir between ray origin and sphere origin
	Vector3 dir = (spherePos - r.GetPosition());

	//Project spherer's origin onto our ray direction vector
	float sphereProj = Vector3::Dot(dir, r.GetDirection());

	if (sphereProj < 0.0f) {
		return false;
	}

	//Get closest point on ray line to sphere
	Vector3 point = r.GetPosition() + (r.GetDirection() * sphereProj);
	float sphereDist = (point - spherePos).Length();

	if (sphereDist > sphereRadius) {
		return false;
	}

	float offset = sqrt((sphereRadius * sphereRadius) - (sphereDist * sphereDist));

	collision.rayDistance = sphereProj - offset;
	collision.collidedAt = r.GetPosition() + (r.GetDirection() * collision.rayDistance);
	return true;
}

Matrix4 GenerateInverseView(const Camera &c) {
	float pitch = c.GetPitch();
	float yaw	= c.GetYaw();
	Vector3 position = c.GetPosition();

	Matrix4 iview =
		Matrix4::Translation(position) *
		Matrix4::Rotation(-yaw, Vector3(0, -1, 0)) *
		Matrix4::Rotation(-pitch, Vector3(-1, 0, 0));

	return iview;
}

Vector3 CollisionDetection::Unproject(const Vector3& screenPos, const Camera& cam) {
	Vector2 screenSize = Window::GetWindow()->GetScreenSize();

	float aspect	= screenSize.x / screenSize.y;
	float fov		= cam.GetFieldOfVision();
	float nearPlane = cam.GetNearPlane();
	float farPlane  = cam.GetFarPlane();

	//Create our inverted matrix! Note how that to get a correct inverse matrix,
	//the order of matrices used to form it are inverted, too.
	Matrix4 invVP = GenerateInverseView(cam) * GenerateInverseProjection(aspect, fov, nearPlane, farPlane);

	//Our mouse position x and y values are in 0 to screen dimensions range,
	//so we need to turn them into the -1 to 1 axis range of clip space.
	//We can do that by dividing the mouse values by the width and height of the
	//screen (giving us a range of 0.0 to 1.0), multiplying by 2 (0.0 to 2.0)
	//and then subtracting 1 (-1.0 to 1.0).
	Vector4 clipSpace = Vector4(
		(screenPos.x / (float)screenSize.x) * 2.0f - 1.0f,
		(screenPos.y / (float)screenSize.y) * 2.0f - 1.0f,
		(screenPos.z),
		1.0f
	);

	//Then, we multiply our clipspace coordinate by our inverted matrix
	Vector4 transformed = invVP * clipSpace;

	//our transformed w coordinate is now the 'inverse' perspective divide, so
	//we can reconstruct the final world space by dividing x,y,and z by w.
	return Vector3(transformed.x / transformed.w, transformed.y / transformed.w, transformed.z / transformed.w);
}

Ray CollisionDetection::BuildRayFromMouse(const Camera& cam) {
	Vector2 screenMouse = Window::GetMouse()->GetAbsolutePosition();
	Vector2 screenSize	= Window::GetWindow()->GetScreenSize();

	//We remove the y axis mouse position from height as OpenGL is 'upside down',
	//and thinks the bottom left is the origin, instead of the top left!
	Vector3 nearPos = Vector3(screenMouse.x,
		screenSize.y - screenMouse.y,
		-0.99999f
	);

	//We also don't use exactly 1.0 (the normalised 'end' of the far plane) as this
	//causes the unproject function to go a bit weird. 
	Vector3 farPos = Vector3(screenMouse.x,
		screenSize.y - screenMouse.y,
		0.99999f
	);

	Vector3 a = Unproject(nearPos, cam);
	Vector3 b = Unproject(farPos, cam);
	Vector3 c = b - a;

	c.Normalise();

	//std::cout << "Ray Direction:" << c << std::endl;

	return Ray(cam.GetPosition(), c);
}

//http://bookofhook.com/mousepick.pdf
Matrix4 CollisionDetection::GenerateInverseProjection(float aspect, float fov, float nearPlane, float farPlane) {
	Matrix4 m;

	float t = tan(fov*PI_OVER_360);

	float neg_depth = nearPlane - farPlane;

	const float h = 1.0f / t;

	float c = (farPlane + nearPlane) / neg_depth;
	float e = -1.0f;
	float d = 2.0f*(nearPlane*farPlane) / neg_depth;

	m.array[0]  = aspect / h;
	m.array[5]  = tan(fov*PI_OVER_360);

	m.array[10] = 0.0f;
	m.array[11] = 1.0f / d;

	m.array[14] = 1.0f / e;

	m.array[15] = -c / (d*e);

	return m;
}

/*
And here's how we generate an inverse view matrix. It's pretty much
an exact inversion of the BuildViewMatrix function of the Camera class!
*/
Matrix4 CollisionDetection::GenerateInverseView(const Camera &c) {
	float pitch = c.GetPitch();
	float yaw	= c.GetYaw();
	Vector3 position = c.GetPosition();

	Matrix4 iview =
Matrix4::Translation(position) *
Matrix4::Rotation(yaw, Vector3(0, 1, 0)) *
Matrix4::Rotation(pitch, Vector3(1, 0, 0));

return iview;
}


/*
If you've read through the Deferred Rendering tutorial you should have a pretty
good idea what this function does. It takes a 2D position, such as the mouse
position, and 'unprojects' it, to generate a 3D world space position for it.

Just as we turn a world space position into a clip space position by multiplying
it by the model, view, and projection matrices, we can turn a clip space
position back to a 3D position by multiply it by the INVERSE of the
view projection matrix (the model matrix has already been assumed to have
'transformed' the 2D point). As has been mentioned a few times, inverting a
matrix is not a nice operation, either to understand or code. But! We can cheat
the inversion process again, just like we do when we create a view matrix using
the camera.

So, to form the inverted matrix, we need the aspect and fov used to create the
projection matrix of our scene, and the camera used to form the view matrix.

*/
Vector3	CollisionDetection::UnprojectScreenPosition(Vector3 position, float aspect, float fov, const Camera &c) {
	//Create our inverted matrix! Note how that to get a correct inverse matrix,
	//the order of matrices used to form it are inverted, too.
	Matrix4 invVP = GenerateInverseView(c) * GenerateInverseProjection(aspect, fov, c.GetNearPlane(), c.GetFarPlane());

	Vector2 screenSize = Window::GetWindow()->GetScreenSize();

	//Our mouse position x and y values are in 0 to screen dimensions range,
	//so we need to turn them into the -1 to 1 axis range of clip space.
	//We can do that by dividing the mouse values by the width and height of the
	//screen (giving us a range of 0.0 to 1.0), multiplying by 2 (0.0 to 2.0)
	//and then subtracting 1 (-1.0 to 1.0).
	Vector4 clipSpace = Vector4(
		(position.x / (float)screenSize.x) * 2.0f - 1.0f,
		(position.y / (float)screenSize.y) * 2.0f - 1.0f,
		(position.z) - 1.0f,
		1.0f
	);

	//Then, we multiply our clipspace coordinate by our inverted matrix
	Vector4 transformed = invVP * clipSpace;

	//our transformed w coordinate is now the 'inverse' perspective divide, so
	//we can reconstruct the final world space by dividing x,y,and z by w.
	return Vector3(transformed.x / transformed.w, transformed.y / transformed.w, transformed.z / transformed.w);
}

bool CollisionDetection::ObjectIntersection(GameObject* a, GameObject* b, CollisionInfo& collisionInfo) {
	const CollisionVolume* volA = a->GetBoundingVolume();
	const CollisionVolume* volB = b->GetBoundingVolume();

	if (!volA || !volB) {
		return false;
	}

	collisionInfo.a = a;
	collisionInfo.b = b;

	Transform& transformA = a->GetTransform();
	Transform& transformB = b->GetTransform();

	VolumeType pairType = (VolumeType)((int)volA->type | (int)volB->type);

	if (pairType == VolumeType::AABB) {
		return AABBIntersection((AABBVolume&)*volA, transformA, (AABBVolume&)*volB, transformB, collisionInfo);
	}

	if (pairType == VolumeType::Sphere) {
		return SphereIntersection((SphereVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}

	if (pairType == VolumeType::OBB) {
		return OBBIntersection((OBBVolume&)*volA, transformA, (OBBVolume&)*volB, transformB, collisionInfo);
	}

	if (pairType == VolumeType::Capsule) {
		return CapsuleIntersection((CapsuleVolume&)*volA, transformA, (CapsuleVolume&)*volB, transformB, collisionInfo);
	}

	if (volA->type == VolumeType::AABB && volB->type == VolumeType::Sphere) {
		return AABBSphereIntersection((AABBVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Sphere && volB->type == VolumeType::AABB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return AABBSphereIntersection((AABBVolume&)*volB, transformB, (SphereVolume&)*volA, transformA, collisionInfo);
	}

	if (volA->type == VolumeType::Capsule && volB->type == VolumeType::Sphere) {
		return SphereCapsuleIntersection((CapsuleVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Sphere && volB->type == VolumeType::Capsule) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return SphereCapsuleIntersection((CapsuleVolume&)*volB, transformB, (SphereVolume&)*volA, transformA, collisionInfo);
	}

	if (volA->type == VolumeType::OBB && volB->type == VolumeType::Sphere) {
		return OBBSphereIntersection((OBBVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}
	if (volA->type == VolumeType::Sphere && volB->type == VolumeType::OBB) {
		collisionInfo.a = b;
		collisionInfo.b = a;
		return OBBSphereIntersection((OBBVolume&)*volA, transformA, (SphereVolume&)*volB, transformB, collisionInfo);
	}

	return false;
}

bool CollisionDetection::AABBTest(const Vector3& posA, const Vector3& posB, const Vector3& halfSizeA, const Vector3& halfSizeB) {
		Vector3 delta = posB - posA;
		Vector3 totalSize = halfSizeA + halfSizeB;
			if (abs(delta.x) < totalSize.x &&
				abs(delta.y) < totalSize.y &&
				abs(delta.z) < totalSize.z) {
			return true;
		}
		return false;
}

//AABB/AABB Collisions
bool CollisionDetection::AABBIntersection(const AABBVolume& volumeA, const Transform& worldTransformA,
	const AABBVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {

	Vector3 boxAPos = worldTransformA.GetPosition();
	Vector3 boxBPos = worldTransformB.GetPosition();
	
	Vector3 boxASize = volumeA.GetHalfDimensions();
	Vector3 boxBSize = volumeB.GetHalfDimensions();

	bool overlap = AABBTest(boxAPos, boxBPos, boxASize, boxBSize);

	if (overlap) {
		static const Vector3 faces[6] = {
			Vector3(-1,0,0), Vector3(1,0,0),
			Vector3(0,-1,0), Vector3(0,1,0),
			Vector3(0,0,-1), Vector3(0,0,1)
		};

		Vector3 maxA = boxAPos + boxASize;
		Vector3 minA = boxAPos - boxASize;

		Vector3 maxB = boxBPos + boxBSize;
		Vector3 minB = boxBPos - boxBSize;

		float distances[6] = {
			(maxB.x - minA.x),// distance of box ’b’ to ’left’ of ’a’.
			(maxA.x - minB.x),// distance of box ’b’ to ’right’ of ’a’.
			(maxB.y - minA.y),// distance of box ’b’ to ’bottom ’ of ’a’.
			(maxA.y - minB.y),// distance of box ’b’ to ’top’ of ’a’.
			(maxB.z - minA.z),// distance of box ’b’ to ’far’ of ’a’.
			(maxA.z - minB.z) // distance of box ’b’ to ’near’ of ’a’.
		};

		float penetration = FLT_MAX;
		Vector3 bestAxis;

		for (int i = 0; i < 6; i++) {
			if (distances[i] < penetration) {
				penetration = distances[i];
				bestAxis = faces[i];
			}
		}
		collisionInfo.AddContactPoint(Vector3(), Vector3(), bestAxis, penetration);
		return true;
	}
	return false;
}

//Sphere / Sphere Collision
bool CollisionDetection::SphereIntersection(const SphereVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	
	float radii = volumeA.GetRadius() + volumeB.GetRadius();
	Vector3 delta = worldTransformB.GetPosition() - worldTransformA.GetPosition();
	float deltaLength = delta.Length();

	if (deltaLength < radii) {
		float penetration = radii - deltaLength;
		Vector3 normal = delta.Normalised();
		Vector3 localA = normal * volumeA.GetRadius();
		Vector3 localB = -normal * volumeB.GetRadius();

		collisionInfo.AddContactPoint(localA, localB, normal, penetration);
		return true;
	}
	return false;
}

//AABB - Sphere Collision
bool CollisionDetection::AABBSphereIntersection(const AABBVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	Vector3 boxSize = volumeA.GetHalfDimensions();
	Vector3 delta = worldTransformB.GetPosition() - worldTransformA.GetPosition();
	Vector3 closestPointOnBox = Maths::Clamp(delta, -boxSize, boxSize);
	Vector3 localPoint = delta - closestPointOnBox;
	float distance = localPoint.Length();

	if (distance < volumeB.GetRadius()) {
		Vector3 collisionNormal = localPoint.Normalised();
		float penetration = (volumeB.GetRadius() - distance);

		Vector3 localA = Vector3();
		Vector3 localB = -collisionNormal * volumeB.GetRadius();

		collisionInfo.AddContactPoint(localA, localB, collisionNormal, penetration);
		return true;
	}
	return false;
}

bool CollisionDetection::OBBIntersection(
	const OBBVolume& volumeA, const Transform& worldTransformA,
	const OBBVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	
	//For every side of both shapes, project the extents onto a vector
	//If there isn't overlap on ANY, they aren't colliding
	//Check along the 3 axis the OBB is going along for BOTH OBBs (6 total)
	//Then check along the cross products between all of these axes (9 total)
	Quaternion rotA = worldTransformA.GetOrientation();
	Quaternion rotB = worldTransformB.GetOrientation();	
	Matrix3 invRotA = Matrix3(rotA.Conjugate());
	Matrix3 invRotB = Matrix3(rotB.Conjugate());
	Vector3 a1 = rotA * Vector3(1, 0, 0);
	Vector3 a2 = rotA * Vector3(0, 1, 0);
	Vector3 a3 = rotA * Vector3(0, 0, 1);
	Vector3 b1 = rotB * Vector3(1, 0, 0);
	Vector3 b2 = rotB * Vector3(0, 1, 0);
	Vector3 b3 = rotB * Vector3(0, 0, 1);

	Vector3 allAxis[15] = {
		a1,a2,a3,b1,b2,b3,
		Vector3::Cross(a1,b1), Vector3::Cross(a1,b2), Vector3::Cross(a1,b3),
		Vector3::Cross(a2,b1), Vector3::Cross(a2,b2), Vector3::Cross(a2,b3),
		Vector3::Cross(a3,b1), Vector3::Cross(a3,b2), Vector3::Cross(a3,b3),
	};

	//Variables to store collision points
	float leastOverlap = FLT_MAX;
	Vector3 leastOverlapAxis;
	Vector3 closestPointA;
	Vector3 closestPointB;

	for(Vector3& v: allAxis) {
		Vector3 aMax = volumeA.SupportFunction(worldTransformA, v);
		Vector3 aMin = volumeA.SupportFunction(worldTransformA, -v);
		Vector3 bMax = volumeB.SupportFunction(worldTransformB, v);
		Vector3 bMin = volumeB.SupportFunction(worldTransformB, -v);
		Vector3 aMaxAlongAxis = v * Vector3::Dot(v, aMax);
		Vector3 aMinAlongAxis = v * Vector3::Dot(v, aMin);
		Vector3 bMaxAlongAxis = v * Vector3::Dot(v, bMax);
		Vector3 bMinAlongAxis = v * Vector3::Dot(v, bMin);

		if ((aMinAlongAxis - bMaxAlongAxis).Length() < (bMinAlongAxis - aMaxAlongAxis).Length()) {
			std::swap(aMaxAlongAxis, bMaxAlongAxis);
			std::swap(aMax, bMax);
			std::swap(aMinAlongAxis, bMinAlongAxis);
			std::swap(aMin, bMin);
		}

		float aLen = (aMinAlongAxis - aMaxAlongAxis).Length();
		float bLen = (bMinAlongAxis - bMaxAlongAxis).Length();


		float totalLength = (aMinAlongAxis - bMaxAlongAxis).Length();

		if (totalLength > aLen + bLen) {
			return false;
		}

		float diffLen = ((aMin - aMax).Length() + (bMin - bMax).Length()) - totalLength;

		if (diffLen < leastOverlap) {
		leastOverlap = diffLen;
		leastOverlapAxis = v;
		closestPointA = aMax;
		closestPointB = bMin;
		}
	}
	Vector3 bestPoint = FindClosestPointOBB(worldTransformA.GetPosition(), worldTransformB.GetPosition(), closestPointA, closestPointB);
	collisionInfo.AddContactPoint((bestPoint - worldTransformA.GetPosition()), (bestPoint - worldTransformB.GetPosition()), leastOverlapAxis, leastOverlap);
	Debug::DrawSphere(bestPoint, 0.5f, Vector3(), 1);
	return true;
}

Vector3 CollisionDetection::FindClosestPointOBB(const Vector3& massCenter1, const Vector3& massCenter2, const Vector3& pointA, const Vector3& pointB) {
	float aSqrDist = (pointA - massCenter1).Length() + (pointA - massCenter2).Length();
	float bSqrDist = (pointB - massCenter1).Length() + (pointB - massCenter2).Length();
	return aSqrDist < bSqrDist ? pointA : pointB;
}

bool CollisionDetection::OBBSphereIntersection(const OBBVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	Quaternion rot = worldTransformA.GetOrientation();
	Matrix3 transform = Matrix3(rot);
	Matrix3 invTransform = Matrix3(rot.Conjugate());

	Vector3 localBoxPos = invTransform * worldTransformA.GetPosition();
	Vector3 boxSize = volumeA.GetHalfDimensions();

	Vector3 localSpherePos = invTransform * worldTransformB.GetPosition();
	Vector3 delta = localSpherePos - localBoxPos;

	Vector3 closestPointOnBox = Maths::Clamp(delta, -boxSize, boxSize);
	Vector3 localPoint = delta - closestPointOnBox;
	float distance = localPoint.Length();

	if (distance < volumeB.GetRadius()) {
		Vector3 collisionNormal = transform * (localPoint.Normalised());
		float penetration = (volumeB.GetRadius() - distance);

		Vector3 localA = transform * closestPointOnBox;
		Vector3 localB = -collisionNormal * volumeB.GetRadius();

		collisionInfo.AddContactPoint(localA, localB, collisionNormal, penetration);
		return true;
	}
	return false;
}

bool CollisionDetection::CapsuleIntersection(
	const CapsuleVolume& volumeA, const Transform& worldTransformA,
	const CapsuleVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	Quaternion rotA = worldTransformA.GetOrientation();
	Matrix3 aInvTrans = Matrix3(rotA.Conjugate());
	Vector3 posA = worldTransformA.GetPosition();	
	Quaternion rotB = worldTransformB.GetOrientation();
	Matrix3 bInvTrans = Matrix3(rotB.Conjugate());
	Vector3 posB = worldTransformB.GetPosition();
	Vector3 aAxis = (rotA * Vector3(0, 1, 0)).Normalised();
	Vector3 bAxis = (rotB * Vector3(0, 1, 0)).Normalised();

	//Closest point on two lines algorithm from: https://wickedengine.net/2020/04/26/capsule-collision-detection/
	Vector3 aTop = posA + aAxis * (volumeA.GetHalfHeight() - volumeA.GetRadius());
	Vector3 aBot = posA - aAxis * (volumeA.GetHalfHeight() - volumeA.GetRadius());
	Vector3 bTop = posB + bAxis * (volumeB.GetHalfHeight() - volumeB.GetRadius());
	Vector3 bBot = posB - bAxis * (volumeB.GetHalfHeight() - volumeB.GetRadius());
	//Vectors between capsule endpoints
	Vector3 bTaT = bTop - aTop;
	Vector3 bBaT = bBot - aTop;
	Vector3 bTaB = bTop - aBot;
	Vector3 bBaB = bBot - aBot;
	//Square Distances
	float bTaTSqrDist = Vector3::Dot(bTaT, bTaT);
	float bBaTSqrDist = Vector3::Dot(bBaT, bBaT);
	float bTaBSqrDist = Vector3::Dot(bTaB, bTaB);
	float bBaBSqrDist = Vector3::Dot(bBaB, bBaB);

	//Finding closest points on vectors a and b
	Vector3 bestA;
	if (bTaBSqrDist < bTaTSqrDist || bTaBSqrDist < bBaTSqrDist 
		|| bBaBSqrDist < bTaTSqrDist || bBaBSqrDist < bBaTSqrDist)
		bestA = aBot;
	else
		bestA = aTop;

	float t = Vector3::Dot((bestA - posB), bAxis);
	Vector3 bestB = posB + bAxis * min(max(t, -1), 1);
	t = Vector3::Dot((bestB - posA), aAxis);
	bestA = posA + aAxis * min(max(t, -1), 1);

	//Sphere-Sphere Collision Detection
	float radii = volumeA.GetRadius() + volumeB.GetRadius();
	Vector3 delta = bestB - bestA;
	float deltaLength = Vector3::Distance(bestA, bestB);

	//If within range, get details of collision by doing a sphere-sphere collision
	if (deltaLength < radii) {
		float penetration = (radii - deltaLength);
		Vector3 normal = delta.Normalised();
		Vector3 localA = bestA - posA + (normal) * volumeA.GetRadius();
		Vector3 localB = bestB - posB - normal * volumeB.GetRadius();
		collisionInfo.AddContactPoint(localA, localB, normal, penetration);
		return true;
	}
	return false;
}

bool CollisionDetection::SphereCapsuleIntersection(
	const CapsuleVolume& volumeA, const Transform& worldTransformA,
	const SphereVolume& volumeB, const Transform& worldTransformB, CollisionInfo& collisionInfo) {
	Quaternion orientation = worldTransformA.GetOrientation();
	Vector3 position = worldTransformA.GetPosition();
	Matrix3 transform = Matrix3(orientation);
	Matrix3 invTransform = Matrix3(orientation.Conjugate());

	Vector3 capsuleUpVec = transform * Vector3(0, 1, 0);
	Vector3 capTop = position + capsuleUpVec * (volumeA.GetHalfHeight() - volumeA.GetRadius());
	Vector3 capBot = position - capsuleUpVec * (volumeA.GetHalfHeight() - volumeA.GetRadius());
	
	//Project sphere's position onto capsuleline using dot product
	Vector3 sphereCenter = worldTransformB.GetPosition();
	float t = Vector3::Dot((sphereCenter - position).Normalised(), capsuleUpVec.Normalised());
	Vector3 closestPointOnLine = position + capsuleUpVec * min(max(t,-1),1);

	float radii = volumeA.GetRadius() + volumeB.GetRadius();
	Vector3 delta = sphereCenter - closestPointOnLine;
	float deltaLength = Vector3::Distance(sphereCenter, closestPointOnLine);
	//If within range, get details of collision by doing a sphere-sphere collision
	if (deltaLength < radii) {
		float penetration = radii - deltaLength;
		Vector3 normal = delta.Normalised();
		Vector3 localA = closestPointOnLine - position + (normal * volumeA.GetRadius());
		Vector3 localB = (-normal * volumeB.GetRadius());

		collisionInfo.AddContactPoint(localA, localB, normal, penetration);
		return true;
	}
	return false;
}