//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _MARBLE_H_
#define _MARBLE_H_

#ifndef _SHAPEBASE_H_
#include "game/shapeBase.h"
#endif

#ifndef _DECALMANAGER_H_
#include "sim/decalManager.h"
#endif

#ifndef _CONCRETEPOLYLIST_H_
#include "collision/concretePolyList.h"
#endif

#ifndef _H_PATHEDINTERIOR
#include "interior/pathedInterior.h"
#endif

#ifndef _STATICSHAPE_H_
#include <game/staticShape.h>
#endif

extern Point3F gMarbleMotionDir;
extern bool gMarbleAxisSet;
extern Point3F gWorkGravityDir;
extern Point3F gMarbleSideDir;

// These might be useful later
//#include <cmath>
//#define CheckNAN(c) { if(isnan(c)) __debugbreak(); }
//#define CheckNAN3(c) { CheckNAN(c.x) CheckNAN(c.y) CheckNAN(c.z) }
//#define CheckNAN3p(c) { CheckNAN(c->x) CheckNAN(c->y) CheckNAN(c->z) }
//#define CheckNANBox(c) { Check3(c.min) Check3(c.min) }
//#define CheckNANBoxp(c) { Check3(c->min) Check3(c->min) }
//#define CheckNANAng(c) { CheckNAN(c.axis.x) CheckNAN(c.axis.y) CheckNAN(c.axis.z) CheckNAN(c.angle) }
//#define CheckNANAngp(c) { CheckNAN(c->axis.x) CheckNAN(c->axis.y) CheckNAN(c->axis.z) CheckNAN(c->angle) }

class MarbleData;
class MarbleUpdateEvent;

class Marble : public ShapeBase
{
    friend class MarbleUpdateEvent;

private:
    typedef ShapeBase Parent;

    friend class ShapeBase;

public:
    enum UpdateMaskBits
    {
        ActiveModeBits = 0x4,
        ActiveModeMask = 0xF,
        ModeBits = 0x7,
        MaxModeTicks = 0x1F
    };

    enum NetMasks {
        MoveMask =     0x8000000,
        WarpMask =    0x10000000,
        PowerUpMask = 0x20000000,
        ModeMask =    0x40000000,
    };
private:

    struct SinglePrecision
    {
        Point3F mPosition;
        Point3F mVelocity;
        Point3F mOmega;
    };

    struct Contact
    {
        SimObject* object;
        Point3D position;
        Point3D normal;
        Point3F actualNormal;
        Point3D surfaceVelocity;
        Point3D surfaceFrictionVelocity;
        F64 staticFriction;
        F64 kineticFriction;
        Point3D vAtC;
        F64 vAtCMag;
        F64 normalForce;
        F64 contactDistance;
        F32 friction;
        F32 restitution;
        F32 force;
        U32 material;
    };

    struct StateDelta
    {
        Point3D pos;
        Point3D posVec;
        F32 prevMouseX;
        F32 prevMouseY;
        Move move;
    };

    struct PowerUpState
    {
        bool active;
        F32 endTime;
        ParticleEmitter* emitter;
    };

    // StaticShape material collisions
    struct MaterialCollision
    {
        U32 ghostIndex;
        U32 materialId;
        NetObject* ghostObject; // MBU says this is NetObject*

        bool operator==(const MaterialCollision& rhs) const {
            return ghostIndex == rhs.ghostIndex &&
                materialId == rhs.materialId &&
                ghostObject == rhs.ghostObject;
        }

        bool operator!=(const MaterialCollision& rhs) const {
            return !(rhs == *this);
        }
    };

    Vector<Contact> mContacts; // 78c
    Contact mBestContact; // 798
    Contact mLastContact; // 860 / 858
    StateDelta delta; // 940 / 930
    MarbleData* mDataBlock; // 9b8 / 9a8
    U32 mPositionKey; // 9bc / 9ac
    U32 mBounceEmitDelay; // 9c4 / 9b4
    U32 mPowerUpId; // 9c8 / 9b8
    U32 mPowerUpTimer; // 9cc / 9b8
    U32 mPrevPowerUpTimer;
    U32 mMode; // 9d4 / 9c4
    //		U32 mModeTimer;
    AUDIOHANDLE mRollHandle; // 9d8 / 9c8
    AUDIOHANDLE mSlipHandle; // 9dc / 9cc
    F32 mRadius; // 9e0 / 9d0
    Point3D mGravityUp; // 9e8 / 9d4

    Point3D mVelocity; // a00 / 9ec
    Point3D mPosition; // a18 / a04
    Point3D mOmega; // a30 / a1c
    F32 mCameraYaw; // a48 / a34
    F32 mCameraPitch; // a4c / a38
    F32 mMouseZ; // a50 / a3c (unused?)
    U32 mGroundTime; // a54 / a40
    bool mControllable; // a58 / a44
    bool mOOB; // a59 / a45
    Point3F mOOBCamPos; // a5c / a48
    SimObjectId mServerMarbleId; // a6c / a58
    SceneObject* mPadPtr; // a74 / a60
    bool mOnPad; // a78 / a64
    Vector<MaterialCollision> mMaterialCollisions; // a7c / a68

    PowerUpState mPowerUpState[6]; // a88 / a74
    ParticleEmitter* mTrailEmitter; // ad0 / abc
    ConcretePolyList mPolyList; // ad4 / ac0
    U32 mPredictionTicks; // c74 / c60
    Point3D mCameraOffset; // c78 / c64
    Point3F mShadowPoints[33]; // c90 / c7c
    bool mShadowGenerated; // e1c / e08

public:
    DECLARE_CONOBJECT(Marble);

    Marble();
    ~Marble();

    static void initPersistFields();
    
    bool onAdd();
    void onRemove();
    void setPosition(const Point3D& pos);
    void setPosition(const Point3D& pos, const AngAxisF& angAxis, float mouseY);
    U32 getPositionKey();
    Point3F getPosition();
    void victorySequence();
    void setMode(U32 mode);
    U32 getMode() { return mMode; }
    void setOOB(bool isOOB);
    void controlPrePacketSend(GameConnection* conn);
    U32 packUpdate(NetConnection* conn, U32 mask, BitStream* stream);
    void unpackUpdate(NetConnection* conn, BitStream* stream); 
    void writePacketData(GameConnection* conn, BitStream* stream);
    void readPacketData(GameConnection* conn, BitStream* stream);
    void bounceEmitter(F32 speed, const Point3F& normal);
    bool onNewDataBlock(GameBaseData* dptr);
    bool updatePadState();
    void trailEmitter(U32 timeDelta);
    void updateRollSound(F32 contactPct, F32 slipAmount);
    void playBounceSound(Marble::Contact& contactSurface, F64 contactVel);
    void setPad(SceneObject* obj);
    inline SceneObject* getPad() { return mPadPtr; }
    void advanceTime(F32 dt);
    void doPowerUp(S32 powerUpId);
    void setPowerUpId(U32 id, bool reset);
    void getCameraTransform(F32* pos, MatrixF* mat);
    void clientStateUpdated(Point3F& position, U32 positionKey, U32 powerUpId, U32 powerUpTimer, Vector<MaterialCollision>& collisions);

    // Marble Physics
    Point3D getVelocityD() const;
    void setVelocityD(const Point3D& vel);
    void setVelocityRotD(const Point3D& rot);
    void applyImpulse(const Point3F& pos, const Point3F& vec);
    void applyContactForces(const Move* move, bool isCentered, Point3D& aControl, const Point3D& desiredOmega, F64 timeStep, Point3D& A, Point3D& a, F32& slipAmount, bool& foundBestContact);
    void getMarbleAxis(Point3D& sideDir, Point3D& motionDir, Point3D& upDir);
    bool computeMoveForces(Point3D& aControl, Point3D& desiredOmega, const Move* move);
    void velocityCancel(bool surfaceSlide, bool noBounce, bool& bouncedYet, bool& stoppedPaths);
    Point3D getExternalForces(const Move* move, F64 timeStep);
    void advancePhysics(const Move* move, U32 timeDelta);
    void advanceCamera(const Move* move, U32 timeDelta);
    void validateEyePoint(float dt, MatrixF* mat);

    // Marble Collision
    bool testMove(Point3D velocity, Point3D& position, F64& deltaT, F64 radius, U32 collisionMask, bool testPIs);
    void findContacts(U32 contactMask);
    void computeFirstPlatformIntersect(F64& dt);

private:
    void setTransform(const MatrixF& mat);
    void renderShadowVolumes(SceneState* state); // TODO

    // Marble Collision
    bool pointWithinPoly(const ConcretePolyList::Poly& poly, const Point3F& point);
    bool pointWithinPolyZ(const ConcretePolyList::Poly& poly, const Point3F& point, const Point3F& upDir);
};

class MarbleData : public ShapeBaseData
{
private:
    typedef ShapeBaseData Parent;

    friend class Marble;

    AudioProfile* rollHardSound; // 314
    AudioProfile* slipSound; // 318
    AudioProfile* bounceSounds[4];
    AudioProfile* jumpSound; // 32c
    F32 maxRollVelocity; // 330
    F32 minVelocityBounceSoft; // 334
    F32 minVelocityBounceHard; // 338
    F32 angularAcceleration; // 33c
    F32 brakingAcceleration; // 340
    F32 staticFriction; // 344
    F32 kineticFriction; // 348
    F32 bounceKineticFriction; // 34c
    F32 gravity; // 350
    F32 maxDotSlide; // 354
    F32 bounceRestitution; // 358
    F32 airAcceleration; // 35c
    F32 energyRechargeRate; // 360
    F32 jumpImpulse; // 364
    F32 cameraDistance; // 368
    U32 maxJumpTicks; // 36c
    F32 maxForceRadius; // 370
    F32 minBounceVel; // 374
    F32 minBounceSpeed; // 378
    F32 minTrailSpeed; // 37c
    ParticleEmitterData* bounceEmitter; // 380
    ParticleEmitterData* trailEmitter; // 384
    ParticleEmitterData* powerUpEmitter[6]; // 388
    U32 powerUpTime[6]; // 3a0

public:
    DECLARE_CONOBJECT(MarbleData);

    MarbleData();

    static void initPersistFields();

    virtual bool preload(bool server, char errorBuffer[256]);
    virtual void packData(BitStream*);
    virtual void unpackData(BitStream*);
};
#endif // _MARBLE_H_
