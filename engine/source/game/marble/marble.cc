#pragma once
#include "game/marble/marble.h"
#include "platform/platform.h"
#include "dgl/dgl.h"
#include "core/bitStream.h"
#include "game/game.h"
#include "math/mMath.h"
#include "console/simBase.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "sim/netConnection.h"
#include "game/item.h"
#include "collision/boxConvex.h"
#include "game/shadow.h"
#include "collision/earlyOutPolyList.h"
#include "collision/extrudedPolyList.h"
#include "math/mathIO.h"
#include "sceneGraph/sceneGraph.h"
#include "game/fx/particleEngine.h"
#include "game/gameConnection.h"
#include <game/trigger.h>

IMPLEMENT_CO_NETOBJECT_V1(Marble);

static Point3F gGlobalGravityDir(0, 0, -1);
static Point3F gPrevGravityDir(0, 0, -1);
static MatrixF gGlobalGravityMatrix(true);
static MatrixF gDefaultGravityTransform(true);
static MatrixF gPrevGravityMatrix(true);
static QuatF   gStartGravityQuat;
static QuatF   gEndGravityQuat;
static bool    gInterpGravityDir = false;
static U32     gInterpCurrentTime;
static U32     gInterpTotalTime;

Marble::Marble() 
{
    Parent();
    mContacts.clear();
    mMaterialCollisions.clear();
    mPolyList.clear();
    mNetFlags.set(0x100);
    mTypeMask |= PlayerObjectType;
    delta.pos = Point3D(0, 0, 0);
    delta.posVec = Point3D(0, 0, 0);
    delta.prevMouseY = 1.0;
    delta.prevMouseX = 0.0;
    delta.move = NullMove;
    mBounceEmitDelay = 0;
    mObjToWorld.setPosition(Point3F(0, 0, 0));
    mPositionKey = 0;
    mVelocity = Point3D(0, 0, 0);
    mCameraOffset = Point3D(0, 1, 0);
    mCameraYaw = 0.0;
    mCameraPitch = 0.0;
    mOmega = Point3D(0, 0, 0);
    mGravityUp = Point3D(0, 0, 1);
    mPosition = Point3D(0, 0, 0);
    mMouseZ = 0.0;
    mControllable = 1;
    mBestContact.normal = Point3D(0, 0, 0);
    mBestContact.actualNormal = Point3D(0, 0, 1);
    mPadPtr = 0;
    mOnPad = 0;
    mShadowGenerated = 0;
    for (int i = 0; i < 6; i++) {
        mPowerUpState[i].active = 0;
        mPowerUpState[i].emitter = 0;
    }
    mTrailEmitter = 0;
    mPowerUpId = 0;
    mPowerUpTimer = 0;
    mPrevPowerUpTimer = -1;
    mGroundTime = 0;
    mRollHandle = 0;
    mSlipHandle = 0;
    mMode = 0;
    mOOB = 0;
}

Marble::~Marble()
{
    for (int i = 0; i < 6; i++)
    {
        if (mPowerUpState[i].emitter)
            mPowerUpState[i].emitter->deleteWhenEmpty();
    }
    if (mTrailEmitter)
        mTrailEmitter->deleteWhenEmpty();
}

bool Marble::onAdd() {
    if (!Parent::onAdd())
        return false;
    addToScene();
    mSceneManager->addShadowOccluder(this);
    if (mNetFlags.test(2))
    {
        mDataBlock->rollHardSound->mDescriptionObject->mDescription.mVolume = 0;
        mRollHandle = alxPlay(mDataBlock->rollHardSound, &mObjToWorld);
        mDataBlock->slipSound->mDescriptionObject->mDescription.mVolume = 0;
        mSlipHandle = alxPlay(mDataBlock->slipSound, &mObjToWorld);
    }
    else
    {
        mPositionKey++;
    }
    return true;
}

void Marble::setPowerUpId(U32 id, bool reset)
{
    mPowerUpId = id;
    if (reset) 
    {
        mPrevPowerUpTimer = mPowerUpTimer;
        mPowerUpTimer++;
    }
    else 
    {
        mPowerUpTimer++;
    }
    setMaskBits(PowerUpMask);
}

void Marble::getCameraTransform(F32* dt, MatrixF* mat)
{
    if (mOOB)
    {
        float offScalar = mRadius + 0.09;
        Point3F camForwardDir = -gPrevGravityDir * offScalar + mRenderObjToWorld.getPosition() - mOOBCamPos;
        m_point3F_normalize(camForwardDir);
        Point3F camSideDir;
        Point3F camUpDir;
        mCross(camForwardDir, -gPrevGravityDir, &camSideDir);
        mCross(camSideDir, camForwardDir, &camUpDir);
        m_point3F_normalize(camSideDir);
        m_point3F_normalize(camUpDir);
        mat->identity();
        mat->setColumn(0, camSideDir);
        mat->setColumn(1, camForwardDir);
        mat->setColumn(2, camUpDir);
        mat->setColumn(3, mOOBCamPos);
    }
    else
    {
        Point3F camSideDir;
        Point3F camUpDir;
        mCross(mCameraOffset, -gPrevGravityDir, &camSideDir);
        mCross(camSideDir, mCameraOffset, &camUpDir);
        m_point3F_normalize(camSideDir);
        m_point3F_normalize(camUpDir);
        mat->setColumn(0, camSideDir);
        mat->setColumn(1, mCameraOffset);
		mat->setColumn(2, camUpDir);
        mat->setColumn(3, Point3F(0, 0, 0));
        Point3D curPos = mRenderObjToWorld.getPosition();
        float offScalar = mRadius + 0.09;
        Point3F camStart = curPos + camUpDir * offScalar;
        Point3F camEnd = mCameraOffset * -mDataBlock->cameraDistance + camStart;
        disableCollision();
        Point3D pos = camStart;
        F64 dt = 1.0;
        Point3D diff = camStart - curPos;
        if (testMove(diff, curPos, dt, 0.09, 90396, true))
            curPos = mRenderObjToWorld.getPosition();
        pos = curPos;
        Point3D diff2 = camEnd - curPos;
        dt = 1.0;
        U32 iters = 0;
        F64 minDt = 1.0;
        do
        {
            Point3D newDiff = diff2;
            if (!testMove(newDiff, pos, dt, 0.09, 90396, 1))
                break;
            minDt -= dt;
            dt = minDt;
            iters++;
			diff2 -= mLastContact.normal * mDot(mLastContact.normal, diff2);
            if (diff2.len() < 0.001)
                break;
        } while (minDt > 0.0 && iters != 4);
        mOOBCamPos = pos;
        Point3F camForward = camStart - pos;
        m_point3F_normalize(camForward);
        mCross(camForward, camUpDir, &camSideDir);
        mCross(camSideDir, camForward, &camUpDir);
        m_point3F_normalize(camSideDir);
        m_point3F_normalize(camUpDir);
        mat->setColumn(0, camSideDir);
        mat->setColumn(1, camForward);
        mat->setColumn(2, camUpDir);
        mat->setColumn(3, pos);
        mat->setRow(3, Point4F(0, 0, 0, 1));
        enableCollision();
    }
}

void Marble::clientStateUpdated(Point3F& position, U32 positionKey, U32 powerUpId, U32 powerUpTimer, Vector<MaterialCollision>& collisions)
{
    if (mPowerUpTimer == powerUpTimer && mPowerUpId != powerUpId)
    {
        mPowerUpId = 0;
        mPowerUpTimer = powerUpTimer + 1;
        Con::executef(this, 1, "onPowerUpUsed");
    }
    if (mPositionKey == positionKey)
    {
        MatrixF prevTransform = mObjToWorld;
        prevTransform.setPosition(position);
        Parent::setTransform(prevTransform);
        Point3F oldPos = mPosition;
        mPosition = position;
        float radiusExpansion = mRadius + 0.2;
        float expansion = this->mRadius + 0.2000000029802322;
        Point3F in_rMax(fmax(oldPos.x, mPosition.x) + expansion, fmax(oldPos.y, mPosition.y) + expansion, fmax(oldPos.z, mPosition.z) + expansion);
        Point3F in_rMin(fmin(oldPos.x, mPosition.x) - expansion, fmin(oldPos.y, mPosition.y) - expansion, fmin(oldPos.z, mPosition.z) - expansion);
        Box3F searchBox(in_rMin, in_rMax);
        SimpleQueryList queryList;
        mContainer->findObjects(searchBox, ItemObjectType | TriggerObjectType, SimpleQueryList::insertionCallback, &queryList);
        for (int i = 0; i < queryList.mList.size(); i++)
        {
            SceneObject* so = queryList.mList[i];
            if ((so->getTypeMask() & TriggerObjectType) != 0)
            {
                Trigger* trig = (Trigger*)so;
                trig->potentialEnterObject(this);
            }
            else if ((so->getTypeMask() & ItemObjectType) != 0 && this != ((Item*)so)->getCollisionObject())
            {
                queueCollision((ShapeBase*)so, VectorF(), 0);
            }
        }
        for (int i = 0; i < collisions.size(); i++)
        {
            MaterialCollision& col = collisions[i];
            if (col.ghostObject)
            {
                ShapeBase* sb = dynamic_cast<ShapeBase*>(col.ghostObject);
                if (sb)
                {
                    queueCollision(sb, VectorF(), col.materialId);
                }
            }
        }
        notifyCollision();
    }
    if (mPadPtr)
    {
        bool prevPad = mOnPad;
        if (updatePadState())
        {
            if (!prevPad)
            {
                Con::executef(mDataBlock, 2, "onEnterPad", scriptThis());
            }
        }
        else if (prevPad)
        {
            Con::executef(mDataBlock, 2, "onLeavePad", scriptThis());
        }
    }
}

bool Marble::onNewDataBlock(GameBaseData* dptr)
{
    if (!Parent::onNewDataBlock(dptr))
        return false;
    mDataBlock = dynamic_cast<MarbleData*>(dptr);
    if (!mDataBlock)
        return false;
    if (mDataBlock->shape.isNull())
        return false;
    mObjBox = (*mDataBlock->shape).bounds;
    Point3F diff = mObjBox.max - mObjBox.min;
    mObjBox.min = diff * -0.5;
    mObjBox.max = diff * 0.5;
    mRadius = mObjBox.max.x;
    resetWorldBox();
    return true;
}

void Marble::onRemove() {
    removeFromScene();
    mSceneManager->removeShadowOccluder(this);
    if (mRollHandle)
        alxStop(mRollHandle);
    if (mSlipHandle)
        alxStop(mSlipHandle);
    Parent::onRemove();
}

bool Marble::updatePadState()
{
    Box3F pad(mPadPtr->getWorldBox());
    MatrixF padTransform = mPadPtr->getTransform();
    Point3F upDir;
    padTransform.getColumn(2, &upDir);
    upDir *= 10;
    if (upDir.x <= 0.0)
        pad.min.x = pad.min.x + upDir.x;
    else
        pad.max.x = pad.max.x + upDir.x;
    if (upDir.y <= 0.0)
        pad.min.y = pad.min.y + upDir.y;
    else
        pad.max.y = pad.max.y + upDir.y;
    if (upDir.z <= 0.0)
        pad.min.z = pad.min.z + upDir.z;
    else
        pad.max.z = pad.max.z + upDir.z;

    bool result;
    result = mWorldBox.isOverlapped(pad);
    if (result)
    {
        Box3F box;
        box.min = this->mObjBox.min + this->mPosition - Point3F(0, 0, 10);
        box.max = this->mObjBox.max + this->mPosition + Point3F(0, 0, 10);
        Point3F boxCenter;
        box.getCenter(&boxCenter);

        float in_rRadius;
        in_rRadius = (box.max - boxCenter).len();
        SphereF sphere(boxCenter, in_rRadius);
        mPolyList.clear();
        mPadPtr->buildPolyList(&mPolyList, box, sphere);
        if (!mPolyList.mPolyList.empty())
        {
            int i = 0;
            for (i = 0; i < mPolyList.mPolyList.size(); i++)
            {
                auto& poly = mPolyList.mPolyList[i];

                if (mDot(poly.plane, upDir * -10) < 0.0)
                {
                    F32 dist = poly.plane.distToPlane(boxCenter);
                    if (dist >= 0.0 && dist < 5.0 && pointWithinPolyZ(poly, boxCenter, upDir))
                        break;
                }
            }
            if (i >= mPolyList.mPolyList.size())
            {
                this->mOnPad = false;
                result = false;
            }
            else
            {
                this->mOnPad = true;
                result = true;
            }
        }
        else
        {
            this->mOnPad = false;
            result = false;
        }
    }
    return result;
}

Point3D Marble::getVelocityD() const
{
    return mVelocity;
}

void Marble::setVelocityD(const Point3D& vel)
{
    mVelocity = vel;
    return setMaskBits(MoveMask | WarpMask);
}

void Marble::setVelocityRotD(const Point3D& vel)
{
    mOmega = vel;
    return setMaskBits(MoveMask | WarpMask);
}

void Marble::initPersistFields()
{
    Parent::initPersistFields();
    addField("Controllable", TypeBool, Offset(mControllable, Marble));
}

void Marble::applyImpulse(const Point3F& pos, const Point3F& vec)
{
    setVelocityD(vec / mMass + mVelocity);
}

void Marble::trailEmitter(U32 timeDelta)
{
    if (!mDataBlock->trailEmitter)
        return;

    F32 speed = mVelocity.len();

    if (mDataBlock->minTrailSpeed > speed)
    {
        if (!mTrailEmitter)
            return;
        mTrailEmitter->deleteWhenEmpty();
        mTrailEmitter = NULL;

        return;
    }

    if (!mTrailEmitter)
    {
        mTrailEmitter = new ParticleEmitter;
        mTrailEmitter->onNewDataBlock(mDataBlock->trailEmitter);
        mTrailEmitter->registerObject();

        if (!mTrailEmitter)
            return;
    }

    if (mDataBlock->minTrailSpeed * 2 > speed)
        timeDelta = (U32)((speed - mDataBlock->minTrailSpeed) / mDataBlock->minTrailSpeed * (F32)timeDelta);

    Point3F normal = mVelocity;
    m_point3F_normalize(normal);

    mTrailEmitter->emitParticles(mPosition, true, normal, mVelocity, timeDelta);
}

void Marble::updateRollSound(F32 contactPct, F32 slipAmount)
{
    if (mRollHandle && mSlipHandle)
    {
        Point3F position = mPosition;
        alxSourcefv(mRollHandle, AL_POSITION, (ALfloat*)&position);
        alxSourcefv(mSlipHandle, AL_POSITION, (ALfloat*)&position);

        Point3D rollVel = mVelocity - mBestContact.surfaceVelocity;
        F32 rollVelLen = rollVel.len();
        float scale = rollVelLen / mDataBlock->maxRollVelocity;
        float rollVolume_1 = fminf(1.0, scale + scale);
        if (contactPct < 0.05)
            rollVolume_1 = 0.0;
        float rollPitch_2 = rollVolume_1 * mDataBlock->rollHardSound->mDescriptionObject->mDescription.mVolume;
        float slipPitch_1;
        float slipVolume;
        float rollVolume;

        if (slipAmount <= 0.0)
        {
            slipPitch_1 = 0.0;
        }
        else
        {
            slipVolume = slipAmount / 5.0;
            if ((float)(slipAmount / 5.0) <= 1.0)
            {
                rollVolume = 1.0 - slipVolume;
            }
            else
            {
                slipVolume = 1.0;
                rollVolume = 0.0;
            }
            rollPitch_2 = rollPitch_2 * rollVolume;
            slipPitch_1 = (float)(slipVolume * mDataBlock->slipSound->mDescriptionObject->mDescription.mVolume);
        }
        float rollPitch_1 = rollPitch_2 * 0.91;
        alxSourcef(mRollHandle, AL_GAIN_LINEAR, rollPitch_1);
        float slipPitch = slipPitch_1 * 0.6;
        alxSourcef(mSlipHandle, AL_GAIN_LINEAR, slipPitch);
        float pitch;
        if (scale <= 1.0)
            pitch = scale;
        else
            pitch = 1.0;
        float rollPitch = pitch * 0.75 + 0.75;
        alxSourcef(mRollHandle, AL_PITCH, rollPitch);

    }
}

void Marble::bounceEmitter(F32 speed, const Point3F& normal)
{
    if (!mBounceEmitDelay)
    {
        if (mDataBlock->bounceEmitter)
        {
            if (mDataBlock->minBounceSpeed <= speed)
            {
                ParticleEmitter* emitter = new ParticleEmitter;
                emitter->onNewDataBlock(mDataBlock->bounceEmitter);
                emitter->registerObject();

                emitter->emitParticles(mPosition, false, normal, mVelocity, speed * 100.0f);

                emitter->deleteWhenEmpty();
                mBounceEmitDelay = 300;
            }
        }
    }
}

void Marble::playBounceSound(Marble::Contact& contact, F64 contactVel)
{
	if (mDataBlock->minVelocityBounceSoft <= contactVel)
	{
		F32 random = Platform::getRandom() * 3.9999;
		int chance = floor(random);
		if (chance > 3)
		{
			Con::printf("Fucky! %d", chance + 2);
		}
		else
		{
			AudioProfile* snd = mDataBlock->bounceSounds[chance];
			float volume = 1.0;
			float oldVolume = snd->mDescriptionObject->mDescription.mVolume;
			if (contactVel < mDataBlock->minVelocityBounceHard)
				volume = (contactVel - mDataBlock->minVelocityBounceSoft) / (mDataBlock->minVelocityBounceHard - mDataBlock->minVelocityBounceSoft);
			snd->mDescriptionObject->mDescription.mVolume = volume * oldVolume;
			alxPlay(snd, &mObjToWorld, NULL);
			snd->mDescriptionObject->mDescription.mVolume = oldVolume;
		}
	}
}


void Marble::setTransform(const MatrixF& mat)
{
    setPosition(mat.getPosition());
}

void Marble::renderShadowVolumes(SceneState* state)
{
}

bool Marble::pointWithinPoly(const ConcretePolyList::Poly& poly, const Point3F& point)
{
    if (poly.vertexCount == 0)
        return true;

    Point3F lastVert = mPolyList.mVertexList[mPolyList.mIndexList[poly.vertexStart + poly.vertexCount - 1]];

    for (int i = 0; i < poly.vertexCount; i++)
    {
        Point3F& v = mPolyList.mVertexList[mPolyList.mIndexList[i + poly.vertexStart]];
        PlaneF p(v + poly.plane, v, lastVert);
        lastVert = v;
        if (p.distToPlane(point) < 0.0f)
            return false;
    }
    return true;
}

bool Marble::pointWithinPolyZ(const ConcretePolyList::Poly& poly, const Point3F& point, const Point3F& upDir)
{
    if (poly.vertexCount == 0)
        return true;

    Point3F lastVert = mPolyList.mVertexList[mPolyList.mIndexList[poly.vertexStart + poly.vertexCount - 1]];

    for (int i = 0; i < poly.vertexCount; i++)
    {
        Point3F& v = mPolyList.mVertexList[mPolyList.mIndexList[i + poly.vertexStart]];
        PlaneF p(v + upDir, v, lastVert);
        lastVert = v;
        if (p.distToPlane(point) < -0.003f)
            return false;
    }
    return true;
}

void Marble::victorySequence()
{
    setVelocity(VectorF(0, 0, 0.1));
}


void Marble::setMode(U32 mode)
{
	mMode = mode;
    setMaskBits(ModeMask);
}

void Marble::setOOB(bool o)
{
    mOOB = o;
    setMaskBits(ModeMask);
}

U32 Marble::packUpdate(NetConnection* conn, U32 mask, BitStream* stream)
{
    Parent::packUpdate(conn, mask, stream);

    bool isControl = false;

    // if it's the control object and the WarpMask is not set
    if (getControllingClient() == (GameConnection*)conn && (mask & WarpMask) == 0)
        isControl = true;

    if (stream->writeFlag(mask & 1))
        stream->write(mId);
    
    if (stream->writeFlag(mask & PowerUpMask) != 0)
    {
        stream->write(mPowerUpId);
        stream->write(mPowerUpTimer);
        stream->writeFlag(mPowerUpTimer == mPrevPowerUpTimer + 1);
    }
    stream->writeInt(mMode, 3);
    stream->writeFlag(mOOB);
    if (stream->writeFlag(isControl))
        return 0;
    if (!isControl)
    {
        if (conn == (NetConnection*)getControllingClient() && (mask & 0x10000000) != 0)
            mask |= MoveMask;

        stream->write(mPositionKey);
        if (stream->writeFlag(mask != 0))
        {
            stream->writeFlag((mask & WarpMask) != 0);
            stream->write(mCameraYaw);
            stream->write(mCameraPitch);
            mathWrite(*stream, mObjToWorld.getPosition());
            mathWrite(*stream, mVelocity);
            mathWrite(*stream, mOmega);
            delta.move.pack(stream);
        }
    }
    // BitStream::writeFlag(a4, this->data_9c0);
    return 0;
}

void Marble::unpackUpdate(NetConnection* conn, BitStream* stream)
{
    ShapeBase::unpackUpdate(conn, stream);
    if (stream->readFlag())
        stream->read(&mServerMarbleId);
    if (stream->readFlag()) {
        stream->read(&mPowerUpId);
		stream->read(&mPowerUpTimer);
        if (stream->readFlag()) {
            for (int i = 0; i < 6; i++) {
                if (mPowerUpState[i].active) {
                    mPowerUpState[i].active = false;
                    if (mPowerUpState[i].emitter) {
                        mPowerUpState[i].emitter->deleteWhenEmpty();
                        mPowerUpState[i].emitter = NULL;
                    }
                }
            }
        }
    }
    mMode = stream->readInt(3);
    mOOB = stream->readFlag();
    if (stream->readFlag())
        return;
    stream->read(&mPositionKey);
    if (stream->readFlag())
    {
        F32 velLen = mVelocity.len();

        bool warp = stream->readFlag();
        stream->read(&mCameraYaw);
        stream->read(&mCameraPitch);
        Point3F pos;
        mathRead(*stream, &pos);
        mathRead(*stream, &mVelocity);
        mathRead(*stream, &mOmega);
        delta.move.unpack(stream);

        delta.posVec = pos - mPosition;
        if (warp)
        {
            delta.pos.set(0, 0, 0);
            delta.prevMouseX = 0;
            delta.posVec.set(0, 0, 0);
            setPosition(pos);
        }
        else
        {
            // wtf is this tho
			F32 velDist = ((velLen + mVelocity.len()) / 2) * 0.032;
            F32 warpTicks = 3;
            if (velDist > 0.00001)
            {
                warpTicks = delta.posVec.len() / velDist;
                U32 mouseYaw = 0;
                if (warpTicks > 0.5)
                    mouseYaw = fmaxf(1, floorf(warpTicks + 0.5));
                
                delta.prevMouseX = mouseYaw;
                if (mouseYaw)
                {
                    if (mouseYaw > 3) {
                        delta.prevMouseX = 3;
                    }
                    delta.posVec /= mouseYaw;
                }
                else
                {
                    if (delta.prevMouseY == 0.0)
                        delta.pos.set(0, 0, 0);
                    else
                        delta.pos = (mPosition - pos) / delta.prevMouseY;
                    setPosition(pos);
                }
            }
        }
    }
}

void Marble::setPad(SceneObject* o)
{
    mPadPtr = o;
    if (o)
        mOnPad = updatePadState();
    else
        mOnPad = false;
}

void Marble::advanceTime(F32 dt)
{
    Parent::advanceTime(dt);
    Point3D deltaRot = mOmega * dt;
    
    rotateMatrix(mObjToWorld, deltaRot);
}

void Marble::doPowerUp(S32 powerUpId)
{
    Con::printf(" Marble powerUpId to do: %d", powerUpId);
    if (mPowerUpState[powerUpId].emitter)
    {
        mPowerUpState[powerUpId].emitter->deleteWhenEmpty();
        mPowerUpState[powerUpId].emitter = NULL;
    }
    if (mDataBlock->powerUpEmitter[powerUpId])
    {
		ParticleEmitter* emitter = new ParticleEmitter();
        emitter->onNewDataBlock(mDataBlock->powerUpEmitter[powerUpId]);
        emitter->registerObject();
        mPowerUpState[powerUpId].emitter = emitter;
    }
    switch (powerUpId)
    {
        case 1:
            applyImpulse(Point3F(), gGlobalGravityDir * -20);
            mPowerUpState[1].active = 1;
            mPowerUpState[1].endTime = mDataBlock->powerUpTime[1] + Sim::getCurrentTime();
            break;
        case 2: {
            MatrixF mat;
            EulerF p(0, 0, mCameraYaw);
            mat.set(p);
            MatrixF gravM = gPrevGravityMatrix;
            gravM.mul(mat);
            Point3F yThing;
            gravM.getColumn(1, &yThing);
            F32 masslessa = mDot((Point3F)mBestContact.normal, yThing);
            yThing -= mBestContact.normal * masslessa;

            if (mDot(yThing, yThing) != 0.0)
                m_point3F_normalize(yThing);
            else
                yThing.set(1, 0, 0);
            Point3F yBoostThing = yThing * 25;
            applyImpulse(Point3F(), yBoostThing);
            mPowerUpState[2].active = 1;
            mPowerUpState[2].endTime = mDataBlock->powerUpTime[2] + Sim::getCurrentTime();
            break;
        }
        case 3: {
              mPowerUpState[3].active = 1;
              mPowerUpState[3].endTime = mDataBlock->powerUpTime[3] + Sim::getCurrentTime();
              SimObject* probSbImage = Sim::findObject("SuperBounceImage");
              if (probSbImage)
              {
                  ShapeBaseImageData* sbImage = dynamic_cast<ShapeBaseImageData*>(probSbImage);
                  if (sbImage)
                  {
                      StringHandle temp;
                      mountImage(sbImage, 3, true, temp);

                      if (temp.getIndex())
                          gNetStringTable->removeString(temp.getIndex());
                  }
              }
              break;
        }
        case 4: {
            mPowerUpState[4].active = 1;
            mPowerUpState[4].endTime = mDataBlock->powerUpTime[4] + Sim::getCurrentTime();

            SimObject* probShockImage = Sim::findObject("ShockAbsorberImage");
            if (probShockImage)
            {
                ShapeBaseImageData* shockImage = dynamic_cast<ShapeBaseImageData*>(probShockImage);
                if (shockImage)
                {
                    StringHandle temp;
                    mountImage(shockImage, 4, true, temp);

                    if (temp.getIndex())
                        gNetStringTable->removeString(temp.getIndex());
                }
            }
            break;
        }
        case 5: {
            mPowerUpState[5].active = 1;
            mPowerUpState[5].endTime = mDataBlock->powerUpTime[5] + Sim::getCurrentTime();
            SimObject* probHeliImage = Sim::findObject("HelicopterImage");
            if (probHeliImage)
            {
                ShapeBaseImageData* heliImage = dynamic_cast<ShapeBaseImageData*>(probHeliImage);
                if (heliImage)
                {
                    StringHandle temp;
                    mountImage(heliImage, 5, true, temp);

                    if (temp.getIndex())
                        gNetStringTable->removeString(temp.getIndex());
                }
            }
            break;
        }
    }
}

void Marble::setPosition(const Point3D& pos)
{
    mObjToWorld.setPosition(pos);
    mPosition = pos;

    Parent::setTransform(mObjToWorld);
    setRenderTransform(mObjToWorld);

    if (!mNetFlags.test(2))
    {
        mPositionKey++;
        setMaskBits(MoveMask | WarpMask);
    }
}

void Marble::setPosition(const Point3D& pos, const AngAxisF& angAxis, float mouseY)
{
    mObjToWorld.setPosition(pos);
    mPosition = pos;

    Parent::setTransform(mObjToWorld);
    setRenderTransform(mObjToWorld);

    MatrixF m;
    angAxis.setMatrix(&m);

    mCameraYaw = mAtan(m[1], m[5]);
    mCameraPitch = mouseY;

    if (!mNetFlags.test(2))
    {
        mPositionKey++;
        setMaskBits(MoveMask | WarpMask);
    }
}

U32 Marble::getPositionKey()
{
    return mPositionKey;
}

Point3F Marble::getPosition()
{
    return mObjToWorld.getPosition();
}

void Marble::readPacketData(GameConnection* conn, BitStream* stream)
{
    Parent::readPacketData(conn, stream);

    Point3F pos;
    mathRead(*stream, &pos);
    mObjToWorld.setPosition(pos);
    stream->setCompressionPoint(pos);
    mPosition = pos;
    Parent::setTransform(mObjToWorld);
    mathRead(*stream, &mVelocity);
    mathRead(*stream, &mOmega);
    stream->read(&mCameraYaw);
    stream->read(&mCameraPitch);
    stream->read(&mMouseZ);
}

void Marble::writePacketData(GameConnection* conn, BitStream* stream)
{
	Parent::writePacketData(conn, stream);

	Point3F pos = mObjToWorld.getPosition();
	mathWrite(*stream, pos);
    stream->setCompressionPoint(pos);
	mathWrite(*stream, mVelocity);
	mathWrite(*stream, mOmega);
	stream->write(mCameraYaw);
	stream->write(mCameraPitch);
	stream->write(mMouseZ);
}

bool Marble::testMove(Point3D velocity, Point3D& position, F64& deltaT, F64 radius, U32 collisionMask, bool testPIs)
{
    F64 velLen = velocity.len();
    if (velocity.len() < 0.001)
        return false;

    Point3F velocityDir = velocity * (1.0 / velLen);

    Point3F deltaPosition = velocity * deltaT;
    Point3F finalPosition = position + deltaPosition;
    Point3F fVel = velocity;

    // If there is a collision mask
    if (collisionMask != 0)
    {
        // Create a Bounding Box and expand it to include the final position
        Box3F box(mObjBox.min + position - Point3D(0.5f, 0.5f, 0.5f), mObjBox.max + position + Point3D(0.5f, 0.5f, 0.5f));

        if (deltaPosition.x >= 0.0)
            box.max.x += deltaPosition.x;
        else
            box.min.x += deltaPosition.x;

        if (deltaPosition.y >= 0.0)
            box.max.y += deltaPosition.y;
        else
            box.min.y += deltaPosition.y;

        if (deltaPosition.z >= 0.0)
            box.max.z += deltaPosition.z;
        else
            box.min.z += deltaPosition.z;

        SimpleQueryList queryList;
        mContainer->findObjects(box, collisionMask, SimpleQueryList::insertionCallback, &queryList);

        SphereF sphere(mPosition, mRadius);

        mPolyList.clear();
        for (int i = 0; i < queryList.mList.size(); i++) {
            queryList.mList[i]->buildPolyList(&mPolyList, box, sphere);
        }
    }

    F64 finalT = deltaT;
    F64 marbleCollisionTime = finalT;
    Point3F marbleCollisionNormal(0.0f, 0.0f, 1.0f);

    Point3D lastContactPos;

    ConcretePolyList::Poly* contactPoly;

    // Marble on Platform collision
    if (!mPolyList.mPolyList.empty())
    {
        ConcretePolyList::Poly* poly;

        for (S32 index = 0; index < mPolyList.mPolyList.size(); index++)
        {
            poly = &mPolyList.mPolyList[index];

            PlaneF polyPlane = poly->plane;

            // If we're going the wrong direction or not going to touch the plane, ignore...
            if (mDot(polyPlane, velocityDir) > -0.001 || mDot(polyPlane, finalPosition) + polyPlane.d > radius)
                continue;

            // Time until collision with the plane
            F64 collisionTime = (radius - (mDot(polyPlane, Point3F(position)) + polyPlane.d)) / mDot(polyPlane, fVel);

            // Are we going to touch the plane during this time step?
            if (collisionTime >= 0.0 && finalT >= collisionTime)
            {
                U32 lastVertIndex = mPolyList.mIndexList[poly->vertexCount - 1 + poly->vertexStart];
                Point3F lastVert = mPolyList.mVertexList[lastVertIndex];

                Point3F collisionPos = velocity * collisionTime + position;

                U32 i;
                for (i = 0; i < poly->vertexCount; i++)
                {
                    Point3F thisVert = mPolyList.mVertexList[mPolyList.mIndexList[i + poly->vertexStart]];
                    if (thisVert != lastVert)
                    {
                        PlaneF edgePlane(thisVert + polyPlane, thisVert, lastVert);
                        lastVert = thisVert;

                        // if we are on the far side of the edge
                        if (mDot(edgePlane, collisionPos) + edgePlane.d < 0.0)
                            break;
                    }
                }

                bool isOnEdge = i != poly->vertexCount;

                // If we're inside the poly, just get the position
                if (!isOnEdge)
                {
                    finalT = collisionTime;
                    finalPosition = collisionPos;
                    lastContactPos = polyPlane.project(collisionPos);
                    contactPoly = poly;
                    continue;
                }
            }

            // We *might* be colliding with an edge

            Point3F lastVert = mPolyList.mVertexList[mPolyList.mIndexList[poly->vertexCount - 1 + poly->vertexStart]];

            if (poly->vertexCount == 0)
                continue;

            F64 radSq = radius * radius;

            for (S32 iter = 0; iter < poly->vertexCount; iter++)
            {
                Point3D thisVert = mPolyList.mVertexList[mPolyList.mIndexList[iter + poly->vertexStart]];

                Point3D vertDiff = lastVert - thisVert;
                Point3D posDiff = position - thisVert;

                Point3D velRejection = mCross(vertDiff, velocity);
                Point3D posRejection = mCross(vertDiff, posDiff);

                // Build a quadratic equation to solve for the collision time
                F64 a = velRejection.lenSquared();
                F64 halfB = mDot(posRejection, velRejection);
                F64 b = halfB + halfB;

                F64 discriminant = b * b - (posRejection.lenSquared() - vertDiff.lenSquared() * radSq) * (a * 4.0);

                // If it's not quadratic or has no solution, ignore this edge.
                if (a == 0.0 || discriminant < 0.0)
                {
                    lastVert = thisVert;
                    continue;
                }

                F64 oneOverTwoA = 0.5 / a;
                F64 discriminantSqrt = mSqrtD(discriminant);

                // Solve using the quadratic formula
                F64 edgeCollisionTime = (discriminantSqrt - b) * oneOverTwoA;
                F64 edgeCollisionTime2 = (-b - discriminantSqrt) * oneOverTwoA;

                // Make sure the 2 times are in ascending order
                if (edgeCollisionTime2 < edgeCollisionTime)
                {
                    F64 temp = edgeCollisionTime2;
                    edgeCollisionTime2 = edgeCollisionTime;
                    edgeCollisionTime = temp;
                }

                // If the collision doesn't happen on this time step, ignore this edge.
                if (edgeCollisionTime2 <= 0.0001 || finalT <= edgeCollisionTime)
                {
                    lastVert = thisVert;
                    continue;
                }

                // Check if the collision hasn't already happened
                if (edgeCollisionTime >= 0.0)
                {
                    F64 edgeLen = vertDiff.len();

                    Point3D relativeCollisionPos = velocity * edgeCollisionTime + position - thisVert;

                    F64 distanceAlongEdge = mDot(relativeCollisionPos, vertDiff) / edgeLen;

                    // If the collision happens outside the boundaries of the edge, ignore this edge.
                    if (-radius > distanceAlongEdge || edgeLen + radius < distanceAlongEdge)
                    {
                        lastVert = thisVert;
                        continue;
                    }

                    // If the collision is within the edge, resolve the collision and continue.
                    if (distanceAlongEdge >= 0.0 && distanceAlongEdge <= edgeLen)
                    {
                        finalT = edgeCollisionTime;
                        finalPosition = velocity * edgeCollisionTime + position;

                        lastContactPos = vertDiff * (distanceAlongEdge / edgeLen) + thisVert;
                        contactPoly = poly;

                        lastVert = thisVert;
                        continue;
                    }
                }

                // This is what happens when we collide with a corner

                F64 speedSq = velocity.lenSquared();

                // Build a quadratic equation to solve for the collision time
                Point3D posVertDiff = position - thisVert;
                F64 halfCornerB = mDot(posVertDiff, velocity);
                F64 cornerB = halfCornerB + halfCornerB;

                F64 fourA = speedSq * 4.0;

                F64 cornerDiscriminant = cornerB * cornerB - (posVertDiff.lenSquared() - radSq) * fourA;

                // If it's quadratic and has a solution ...
                if (speedSq != 0.0 && cornerDiscriminant >= 0.0)
                {
                    F64 oneOver2A = 0.5 / speedSq;
                    F64 cornerDiscriminantSqrt = mSqrtD(cornerDiscriminant);

                    // Solve using the quadratic formula
                    F64 cornerCollisionTime = (cornerDiscriminantSqrt - cornerB) * oneOver2A;
                    F64 cornerCollisionTime2 = (-cornerB - cornerDiscriminantSqrt) * oneOver2A;

                    // Make sure the 2 times are in ascending order
                    if (cornerCollisionTime2 < cornerCollisionTime)
                    {
                        F64 temp = cornerCollisionTime2;
                        cornerCollisionTime2 = cornerCollisionTime;
                        cornerCollisionTime = temp;
                    }

                    // If the collision doesn't happen on this time step, ignore this corner
                    if (cornerCollisionTime2 > 0.0001 && finalT > cornerCollisionTime)
                    {
                        // Adjust to make sure very small negative times are counted as zero
                        if (cornerCollisionTime <= 0.0 && cornerCollisionTime > -0.0001)
                            cornerCollisionTime = 0.0;

                        // Check if the collision hasn't already happened
                        if (cornerCollisionTime >= 0.0)
                        {
                            // Resolve it and continue
                            finalT = cornerCollisionTime;
                            contactPoly = poly;
                            finalPosition = velocity * cornerCollisionTime + position;
                            lastContactPos = thisVert;
                        }
                    }
                }

                // We still need to check the other corner ...
                // Build one last quadratic equation to solve for the collision time
                Point3D lastVertDiff = position - lastVert;
                F64 lastCornerHalfB = mDot(lastVertDiff, velocity);
                F64 lastCornerB = lastCornerHalfB + lastCornerHalfB;
                F64 lastCornerDiscriminant = lastCornerB * lastCornerB - (lastVertDiff.lenSquared() - radSq) * fourA;

                // If it's not quadratic or has no solution, then skip this corner
                if (speedSq == 0.0 || lastCornerDiscriminant < 0.0)
                {
                    lastVert = thisVert;
                    continue;
                }

                F64 lastCornerOneOver2A = 0.5 / speedSq;
                F64 lastCornerDiscriminantSqrt = mSqrtD(lastCornerDiscriminant);

                // Solve using the quadratic formula
                F64 lastCornerCollisionTime = (lastCornerDiscriminantSqrt - lastCornerB) * lastCornerOneOver2A;
                F64 lastCornerCollisionTime2 = (-lastCornerB - lastCornerDiscriminantSqrt) * lastCornerOneOver2A;

                // Make sure the 2 times are in ascending order
                if (lastCornerCollisionTime2 < lastCornerCollisionTime)
                {
                    F64 temp = lastCornerCollisionTime2;
                    lastCornerCollisionTime2 = lastCornerCollisionTime;
                    lastCornerCollisionTime = temp;
                }

                // If the collision doesn't happen on this time step, ignore this corner
                if (lastCornerCollisionTime2 <= 0.0001 || finalT <= lastCornerCollisionTime)
                {
                    lastVert = thisVert;
                    continue;
                }

                // Adjust to make sure very small negative times are counted as zero
                if (lastCornerCollisionTime <= 0.0 && lastCornerCollisionTime > -0.0001)
                    lastCornerCollisionTime = 0.0;

                // Check if the collision hasn't already happened
                if (lastCornerCollisionTime < 0.0)
                {
                    lastVert = thisVert;
                    continue;
                }

                // Resolve it and continue
                finalT = lastCornerCollisionTime;
                finalPosition = velocity * lastCornerCollisionTime + position;
                lastContactPos = lastVert;
                contactPoly = poly;

                lastVert = thisVert;
            }
        }
    }

    position = finalPosition;

    bool contacted = false;
    if (deltaT > finalT)
    {
        MaterialProperty* material;

        // Did we collide with a poly as opposed to a marble?
        if (marbleCollisionTime > finalT && contactPoly != NULL && contactPoly->material != -1)
        {
            material = contactPoly->object->getMaterialProperty(contactPoly->material);
            if ((contactPoly->object->getTypeMask() & ShapeBaseObjectType) != 0)
            {
                Point3F objVelocity = ((ShapeBase*)contactPoly->object)->getVelocity();
                queueCollision((ShapeBase*)contactPoly->object, mVelocity - objVelocity, contactPoly->material);
            }
        }

        mLastContact.position = lastContactPos;

        // Did we collide with a marble?
        if (marbleCollisionTime <= finalT)
            mLastContact.normal = marbleCollisionNormal;
        else
        {
            // or a poly?
            mLastContact.normal = finalPosition - lastContactPos;
            mLastContact.normal.normalize();
        }

        if (material == NULL)
        {
            mLastContact.friction = 1.0f;
            mLastContact.restitution = 1.0f;
            mLastContact.force = 0.0f;
        }
        else
        {
            mLastContact.friction = material->friction;
            mLastContact.restitution = material->restitution;
            mLastContact.force = material->force;
        }

        contacted = true;
    }

    deltaT = finalT;
    return contacted;
}

void Marble::findContacts(U32 contactMask)
{
    mContacts.clear();

    if (contactMask != 0)
    {
        Box3F box(mPosition, mPosition);
        box.min += mObjBox.min - Point3F(0.0001f, 0.0001f, 0.0001f);
        box.max += mObjBox.max + Point3F(0.0001f, 0.0001f, 0.0001f);

        SimpleQueryList queryList;
        mContainer->findObjects(box, contactMask, SimpleQueryList::insertionCallback, &queryList);

        SphereF sphere(mPosition, mRadius);

        mPolyList.clear();
        for (int i = 0; i < queryList.mList.size(); i++) {
            queryList.mList[i]->buildPolyList(&mPolyList, box, sphere);
        }
    }

    for (int i = 0; i < mPolyList.mPolyList.size(); i++)
    {
        ConcretePolyList::Poly* poly = &mPolyList.mPolyList[i];
        PlaneF plane(poly->plane);
        F64 distance = plane.distToPlane(mPosition);
        if (mFabsD(distance) <= (F64)mRadius + 0.0001) {
            Point3D lastVertex(mPolyList.mVertexList[mPolyList.mIndexList[poly->vertexStart + poly->vertexCount - 1]]);

            Point3D contactVert = plane.project(mPosition);
            Point3D finalContact = contactVert;
            F64 separation = mSqrtD(mRadius * mRadius - distance * distance);

            for (int j = 0; j < poly->vertexCount; j++) {
                Point3D vertex = mPolyList.mVertexList[mPolyList.mIndexList[poly->vertexStart + j]];
                if (vertex != lastVertex) {
                    PlaneF vertPlane(vertex + plane, vertex, lastVertex);
                    F64 vertDistance = vertPlane.distToPlane(contactVert);
                    if (vertDistance < 0.0) {
                        if (vertDistance < -(separation + 0.0001))
                            goto superbreak;

                        if (PlaneF(vertPlane + vertex, vertex, vertex + plane).distToPlane(contactVert) >= 0.0) {
                            if (PlaneF(lastVertex - vertPlane, lastVertex, lastVertex + plane).distToPlane(contactVert) >= 0.0) {
                                finalContact = vertPlane.project(contactVert);
                                break;
                            }
                            finalContact = lastVertex;
                        }
                        else {
                            finalContact = vertex;
                        }
                    }
                    lastVertex = vertex;
                }
            }

            MaterialProperty* matProp = poly->object->getMaterialProperty(poly->material);

            PathedInterior* hitPI = dynamic_cast<PathedInterior*>(poly->object);

            Point3D surfaceVelocity;
            if (hitPI != nullptr) {
                surfaceVelocity = hitPI->getVelocity();
            }
            else {
                surfaceVelocity = Point3D(0, 0, 0);
            }

            U32 materialId = poly->material;
            Point3D delta = mPosition - finalContact;
            F64 contactDistance = delta.len();
            if ((F64)mRadius + 0.0001 < contactDistance) {
                continue;
            }

            Point3D normal(plane.x, plane.y, plane.z);
            if (contactDistance != 0.0) {
                normal = delta * (1.0 / contactDistance);
            }
            F32 force = 0.0;
            F32 friction = 1.0;
            F32 restitution = 1.0;
            if (matProp != nullptr) {
                friction = matProp->friction;
                restitution = matProp->restitution;
                force = matProp->force;
            }

            Marble::Contact contact{};

            contact.restitution = restitution;
            contact.normal = normal;
            contact.position = finalContact;
            contact.surfaceVelocity = surfaceVelocity;
            contact.object = poly->object;
            contact.contactDistance = contactDistance;
            contact.friction = friction;
            contact.force = force;
            contact.material = materialId;

            mContacts.push_back(contact);

            GameBase* gb = dynamic_cast<GameBase*>(poly->object);
            U32 objTypeMask = 0;
            if (gb != nullptr) {
                objTypeMask = gb->getTypeMask();
            }

            if ((objTypeMask & ShapeBaseObjectType) != 0) {
                U32 netIndex = gb->getNetIndex();

                bool found = false;
                for (int j = 0; j < mMaterialCollisions.size(); j++) {
                    if (mMaterialCollisions[j].ghostIndex == netIndex && mMaterialCollisions[j].materialId == materialId) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    Marble::MaterialCollision coll{};
                    coll.ghostIndex = netIndex;
                    coll.materialId = materialId;
                    coll.ghostObject = gb;
                    gb->getNetIndex();
                    mMaterialCollisions.push_back(coll);
                    Point3F offset(0, 0, 0);
                    queueCollision(reinterpret_cast<ShapeBase*>(gb), offset, materialId);
                }
            }
        }
    superbreak:
        (void)0;
    }
}

void Marble::computeFirstPlatformIntersect(F64& dt)
{
    Box3F box;
    box.min = mPosition + mObjBox.min - Point3F(0.5f, 0.5f, 0.5f);
    box.max = mPosition + mObjBox.max + Point3F(0.5f, 0.5f, 0.5f);

    Point3F deltaVelocity = mVelocity * dt;
    if (deltaVelocity.x >= 0.0f)
        box.max.x += deltaVelocity.x;
    else
        box.min.x += deltaVelocity.x;

    if (deltaVelocity.y >= 0.0f)
        box.max.y += deltaVelocity.y;
    else
        box.min.y += deltaVelocity.y;

    if (deltaVelocity.z >= 0.0f)
        box.max.z += deltaVelocity.z;
    else
        box.min.z += deltaVelocity.z;

    for (auto it = PathedInterior::getClientPathedInteriors(); it; it = it->getNext())
    {
        Box3F itBox = it->getExtrudedBox();
        if (itBox.isOverlapped(box))
        {
            bool isContacting = false;
            for (S32 j = 0; j < mContacts.size(); j++)
            {
                if (mContacts[j].object != it)
                    continue;

                isContacting = true;
                break;
            }

            if (!isContacting)
            {
                Point3F vel = mVelocity - it->getVelocity();
                Point3F boxCenter;
                itBox.getCenter(&boxCenter);

                Point3F diff = itBox.max - boxCenter;
                SphereF sphere(boxCenter, diff.len());
                mPolyList.clear();
                it->buildPolyList(&mPolyList, itBox, sphere);

                Point3D position = mPosition;
                testMove(vel, position, dt, mRadius, 0, false);
            }
        }
    }
}

void Marble::applyContactForces(const Move* move, bool isCentered, Point3D& aControl, const Point3D& desiredOmega, F64 timeStep, Point3D& A, Point3D& a, F32& slipAmount, bool& foundBestContact)
{
    F32 force = 0.0f;
    S32 bestContactIndex = mContacts.size();
    for (S32 i = 0; i < mContacts.size(); i++)
    {
        Contact* contact = &mContacts[i];
        contact->normalForce = -mDot(contact->normal, A);

        if (contact->normalForce > force)
        {
            force = contact->normalForce;
            bestContactIndex = i;
        }

    }

    if (bestContactIndex != mContacts.size() && mMode != 1) {
        mBestContact = mContacts[bestContactIndex];
        foundBestContact = true;
    }
    else 
    {
        foundBestContact = false;
    }

    if (move->trigger[2] && bestContactIndex != mContacts.size())
    {
        F64 friction = fmaxf(0, mDot((mVelocity - mBestContact.surfaceVelocity), mBestContact.normal));

        if (mDataBlock->jumpImpulse > friction)
        {
            mVelocity += (mDataBlock->jumpImpulse - friction) * mBestContact.normal;
            mGroundTime = 0;
            if (mDataBlock->jumpSound)
                alxPlay(mDataBlock->jumpSound, &mObjToWorld, NULL);
        }
    }

    for (S32 i = 0; i < mContacts.size(); i++)
    {
        Contact& contact = mContacts[i];

        F64 normalForce = -mDot(contact.normal, A);

        if (normalForce > 0.0 &&
            mDot(mVelocity - contact.surfaceVelocity, contact.normal) <= 0.0001)
        {
            A += contact.normal * normalForce;
        }
    }

    if (bestContactIndex != mContacts.size() && mMode != 1)
    {
        Point3D vAtC = mVelocity + mCross(mOmega, -mBestContact.normal * mRadius) - mBestContact.surfaceVelocity;
        mBestContact.vAtCMag = vAtC.len();

        bool slipping = false;
        Point3D aFriction(0, 0, 0);
        Point3D AFriction(0, 0, 0);

        if (mBestContact.vAtCMag != 0.0)
        {
            slipping = true;

            F32 friction = 0.0f;
            if (mMode != 2)
                friction = mDataBlock->kineticFriction * mBestContact.friction;

            F64 angAMagnitude = friction * 5.0 * mBestContact.normalForce / (mRadius + mRadius);
            F64 AMagnitude = mBestContact.normalForce * friction;
            F64 totalDeltaV = (mRadius * angAMagnitude + AMagnitude) * timeStep;
            if (mBestContact.vAtCMag < totalDeltaV)
            {
                slipping = false;
                angAMagnitude *= mBestContact.vAtCMag / totalDeltaV;
                AMagnitude *= mBestContact.vAtCMag / totalDeltaV;
            }

            Point3D vAtCDir = vAtC / mBestContact.vAtCMag;

            aFriction = mCross(-mBestContact.normal, -vAtCDir) * angAMagnitude;
            AFriction = -AMagnitude * vAtCDir;

            slipAmount = mBestContact.vAtCMag - totalDeltaV;
        }

        if (!slipping)
        {
            Point3D R = -gGlobalGravityDir * mRadius;
            Point3D aadd = mCross(R, A) / R.lenSquared();

            if (isCentered)
            {
                Point3D nextOmega = mOmega + a * timeStep;
                aControl = desiredOmega - nextOmega;

                F64 aScalar = aControl.len();
                if (mDataBlock->brakingAcceleration < aScalar)
                    aControl *= mDataBlock->brakingAcceleration / aScalar;
            }

            Point3D Aadd = -mCross(aControl, -mBestContact.normal * mRadius);
            F64 aAtCMag = (mCross(aadd, -mBestContact.normal * mRadius) + Aadd).len();

            F64 friction2 = 0.0;
            if (mMode != 2)
                friction2 = mDataBlock->staticFriction * mBestContact.friction;

            if (friction2 * mBestContact.normalForce < aAtCMag)
            {
                friction2 = 0.0;
                if (mMode != 2)
                    friction2 = mDataBlock->kineticFriction * mBestContact.friction;

                Aadd *= friction2 * mBestContact.normalForce / aAtCMag;
            }

            A += Aadd;
            a += aadd;
        }

        A += AFriction;
        a += aFriction;
    }

    a += aControl;
}

void Marble::getMarbleAxis(Point3D& sideDir, Point3D& motionDir, Point3D& upDir)
{
    mGravityUp = -gGlobalGravityDir;
    sideDir = mCross(mCameraOffset, mGravityUp);
    m_point3D_normalize(sideDir);
    motionDir = mCross(mGravityUp, sideDir);
    m_point3D_normalize(motionDir);
    upDir = mGravityUp;
}

bool Marble::computeMoveForces(Point3D& aControl, Point3D& desiredOmega, const Move* move)
{
    aControl.set(0, 0, 0);
    desiredOmega.set(0, 0, 0);

    Point3F invGrav = -gGlobalGravityDir;

    Point3D r = invGrav * mRadius;

    Point3D rollVelocity;
    mCross(mOmega, r, rollVelocity);

    Point3D sideDir;
    Point3D motionDir;
    Point3D upDir;
    getMarbleAxis(sideDir, motionDir, upDir);

    Point2F currentVelocity(mDot(sideDir, rollVelocity), mDot(motionDir, rollVelocity));

    Point2F mv(move->x, move->y);

    Point2F desiredVelocity = mv * mDataBlock->maxRollVelocity;

    if (desiredVelocity.x != 0.0f || desiredVelocity.y != 0.0f)
    {
        if (currentVelocity.y > desiredVelocity.y && desiredVelocity.y > 0) {
            desiredVelocity.y = currentVelocity.y;
        }
        else if (currentVelocity.y < desiredVelocity.y && desiredVelocity.y < 0) {
            desiredVelocity.y = currentVelocity.y;
        }
        if (currentVelocity.x > desiredVelocity.x && desiredVelocity.x > 0) {
            desiredVelocity.x = currentVelocity.x;
        }
        else if (currentVelocity.x < desiredVelocity.x && desiredVelocity.x < 0) {
            desiredVelocity.x = currentVelocity.x;
        }

        Point3D newMotionDir = sideDir * desiredVelocity.x + motionDir * desiredVelocity.y;

        Point3D newSideDir;
        mCross(r, newMotionDir, newSideDir);

        desiredOmega = newSideDir * (1.0f / r.lenSquared());
        aControl = desiredOmega - mOmega;

        if (mDataBlock->angularAcceleration < aControl.len())
            aControl *= mDataBlock->angularAcceleration / aControl.len();

        return false;
    }

    return true;
}

void Marble::velocityCancel(bool surfaceSlide, bool noBounce, bool& bouncedYet, bool& stoppedPaths)
{
    bool looped = false;
    U32 itersIn = 0;
    bool done;

    do
    {
        done = true;
        itersIn++;

        for (S32 i = 0; i < mContacts.size(); i++)
        {
            Contact* contact = &mContacts[i];

            Point3D sVel = mVelocity - contact->surfaceVelocity;
            F64 surfaceDot = mDot(contact->normal, sVel);

            if ((!looped && surfaceDot < 0.0) || surfaceDot < -0.001)
            {
                F64 velLen = mVelocity.len();
                Point3D surfaceVel = contact->normal * surfaceDot;

                if (!bouncedYet)
                {
                    playBounceSound(*contact, -surfaceDot);
                    bouncedYet = true;
                }

                if (noBounce)
                {
                    mVelocity -= surfaceVel;
                }
                else
                {
                    if (contact->surfaceVelocity.len() == 0.0 && !surfaceSlide && surfaceDot > -mDataBlock->maxDotSlide * velLen)
                    {

                        mVelocity -= surfaceVel;
                        m_point3D_normalize(mVelocity);
                        mVelocity *= velLen;
                        surfaceSlide = true;
                    }
                    else if (surfaceDot >= -mDataBlock->minBounceVel)
                    {
                        mVelocity -= surfaceVel;
                    }
                    else
                    {
                        F64 bounceRestitution;
                        if (mPowerUpState[4].active)
                            bounceRestitution = 0.01;
                        else if (mPowerUpState[3].active)
                            bounceRestitution = 0.9;
                        else
                            bounceRestitution = mDataBlock->bounceRestitution;
                        F64 restitution = contact->restitution * bounceRestitution;
                        Point3D velocityAdd = -(1.0 + restitution) * surfaceVel;
                        Point3D vAtC = sVel + mCross(mOmega, -contact->normal * mRadius);
                        F64 normalVel = -mDot(contact->normal, sVel);

                        bounceEmitter(sVel.len() * restitution, contact->normal);

                        vAtC -= contact->normal * mDot(contact->normal, sVel);

                        F64 vAtCMag = vAtC.len();
                        if (vAtCMag != 0.0) {
                            F64 friction = mDataBlock->bounceKineticFriction * contact->friction;

                            F64 angVMagnitude = friction * 5.0 * normalVel / (mRadius + mRadius);
                            if (vAtCMag / mRadius < angVMagnitude)
                                angVMagnitude = vAtCMag / mRadius;

                            Point3D vAtCDir = vAtC / vAtCMag;

                            Point3D deltaOmega = mCross(-contact->normal, -vAtCDir) * angVMagnitude;
                            mOmega += deltaOmega;

                            mVelocity -= mCross(-deltaOmega, -contact->normal * mRadius);
                        }
                        mVelocity += velocityAdd;
                    }

                }

                done = false;
            }
        }

        looped = true;

        if (itersIn > 6 && !stoppedPaths)
        {
            stoppedPaths = true;
            if (noBounce)
                done = true;

            for (S32 j = 0; j < mContacts.size(); j++)
            {
                mContacts[j].surfaceVelocity.set(0, 0, 0);
            }

            for (auto pi = PathedInterior::getClientPathedInteriors(); pi != NULL; pi = pi->getNext())
            {
                if (pi->getExtrudedBox().isOverlapped(mWorldBox))
                    pi->setStopped();
            }
        }

    } while (!done);
}

Point3D Marble::getExternalForces(const Move* move, F64 timeStep)
{
    if (mMode == 1)
        return mVelocity * -16.0;

    F32 gravity = mDataBlock->gravity;
    if (mPowerUpState[5].active)
        gravity *= 0.25;
    Point3D ret = gGlobalGravityDir * gravity;

    Box3F marbleBox(mPosition - Point3F(mDataBlock->maxForceRadius, mDataBlock->maxForceRadius, mDataBlock->maxForceRadius), 
        mPosition + Point3F(mDataBlock->maxForceRadius, mDataBlock->maxForceRadius, mDataBlock->maxForceRadius));

    SimpleQueryList sql;
    mContainer->findObjects(marbleBox, ForceObjectType, SimpleQueryList::insertionCallback, &sql);

    Point3F force(0.0f, 0.0f, 0.0f);
    Point3F position = mPosition;

    for (S32 i = 0; i < sql.mList.size(); i++)
    {
        GameBase* obj = (GameBase*)sql.mList[i];
        if (obj != this)
            obj->getForce(position, &force);
    }

    ret += force / mMass;

    S32 forceObjectCount = 0;

    if (!mContacts.empty())
    {
        Point3F contactNormal(0.0f, 0.0f, 0.0f);
        F32 contactForce = 0.0f;

        for (S32 i = 0; i < mContacts.size(); i++)
        {
            if (mContacts[i].force != 0.0f)
            {
                forceObjectCount++;
                contactNormal += mContacts[i].normal;
                contactForce = mContacts[i].force;
            }
        }

        if (forceObjectCount != 0)
        {
            m_point3F_normalize(contactNormal);

            F32 contactForceOverMass = contactForce / mMass;

            F32 thing = mDot((Point3F)mVelocity, contactNormal);
            if (contactForceOverMass > thing)
            {
                if (thing > 0.0f)
                    contactForceOverMass -= thing;

                ret += contactNormal * (contactForceOverMass / timeStep);
            }
        }
    }

    if (mContacts.empty() && mMode != 2)
    {
        Point3D sideDir;
        Point3D motionDir;
        Point3D upDir;
        getMarbleAxis(sideDir, motionDir, upDir);

        Point3F movement = sideDir * move->x + motionDir * move->y;

        float airAcceleration;
        if (mPowerUpState[5].active)
            airAcceleration = mDataBlock->airAcceleration + mDataBlock->airAcceleration;
        else
            airAcceleration = mDataBlock->airAcceleration;
        
        ret += movement * airAcceleration;
    }

    return ret;
}

void Marble::advancePhysics(const Move* move, U32 timeDelta)
{
    if (!timeDelta)
        return;
    if (mBounceEmitDelay)
    {
        if (timeDelta <= mBounceEmitDelay)
            mBounceEmitDelay -= timeDelta;
        else
            mBounceEmitDelay = 0;
    }
    U32 currentTime = Sim::getCurrentTime();
    if (!mControllable || !getControllingClient())
        move = &NullMove;
    advanceCamera(move, timeDelta);
    delta.move = *move;
    delta.pos = mPosition;
    if (move->trigger[0] && mPowerUpId)
    {
        doPowerUp(mPowerUpId);
        mPowerUpId = 0;
    }

    U32 timeRemaining = timeDelta;
    U32 timeStepInt;
    U32 it = 0;
    F64 mMoveTime;
    F64 contactTime = 0.0;
    float slipAmount = 0.0;
    bool bouncedYet = false;
    do
    {
        if (timeRemaining > 8)
        {
            timeRemaining -= 8;
            timeStepInt = 8;
        }
        else
        {
            timeStepInt = timeRemaining;
            timeRemaining = 0;
        }

        for (auto pi = PathedInterior::getClientPathedInteriors(); pi; pi = pi->getNext())
            pi->computeNextPathStep(timeStepInt);

        double timeStep = (double)((long double)timeStepInt) / 1000.0;
        
        if (timeStep != 0.0)
        {
            while (true)
            {
                Point3D aControl;
                Point3D desiredOmega;

                bool isCentered = computeMoveForces(aControl, desiredOmega, move);
                findContacts(0x601D);

                bool stoppedPaths = false;
                velocityCancel(isCentered, false, bouncedYet, stoppedPaths);

                Point3D A = getExternalForces(move, timeStep);

                Point3D a(0, 0, 0);
                bool foundBestContact = false;
                applyContactForces(move, isCentered, aControl, desiredOmega, timeStep, A, a, slipAmount, foundBestContact);

                mVelocity += A * timeStep;
                mOmega += a * timeStep;

                mMoveTime = timeStep;
                if (mMode == 2)
                {
                    mVelocity.y = 0.0;
                    mVelocity.x = 0.0;
                }

                velocityCancel(isCentered, true, bouncedYet, stoppedPaths);

                computeFirstPlatformIntersect(mMoveTime);
                testMove(mVelocity, mPosition, mMoveTime, mRadius, 0x601D, false);
                F64 moveTime;
                if (timeStep == mMoveTime)
                {
                    moveTime = timeStep;
                }
                else
                {
                    mVelocity -= A * (timeStep - mMoveTime);
                    mOmega -= a * (timeStep - mMoveTime);
                    moveTime = mMoveTime;
                }
                if (mContacts.size())
                    contactTime += moveTime;
                if (foundBestContact)
                {
                    mGroundTime += moveTime * 1000;
                    if (mGroundTime > 250)
                        mGroundTime = 250;
                }
                else
                {
                    if (mGroundTime <= moveTime)
                        mGroundTime = 0;
                    else
                        mGroundTime -= moveTime;
                }
                for (auto pi = PathedInterior::getClientPathedInteriors(); pi; pi = pi->getNext())
                    pi->advance(moveTime * 1000);

                F64 timeDiff = timeStep - moveTime;
                timeStep -= moveTime;
                if (++it > 10 || timeDiff == 0.0)
                    break;
            }
        }
        
    } while (timeRemaining);

    if (move->trigger[1])
    {
        mVelocity *= 0.8;
        setMaskBits(MoveMask | WarpMask);
        mOmega *= 0.8;
        setMaskBits(MoveMask | WarpMask);
    }

    F32 contactPct = contactTime * 1000.0 / timeDelta;
    Con::setFloatVariable("testCount", contactPct);
    Con::setFloatVariable("marblePitch", mCameraPitch);
    setPosition(mPosition);
    trailEmitter(timeDelta);

    F32 currentTime2 = Sim::getCurrentTime();

    U32 mountIndex = 0;
    for (int i = 0; i < 6; i++) {
        if (mPowerUpState[i].active && currentTime2 > mPowerUpState[i].endTime)
        {
            mPowerUpState[i].active = false;
            MatrixF mt;
            unmountImage(i);
            if (mPowerUpState[i].emitter)
            {
                mPowerUpState[i].emitter->deleteWhenEmpty();
                mPowerUpState[i].emitter = NULL;
            }
        }
        if (mPowerUpState[i].emitter) {
            Point3F norm = mVelocity;
            m_point3F_normalize(norm);
            mPowerUpState[i].emitter->emitParticles(mPosition, true, norm, mVelocity, timeDelta);
        }
    }

    if (mOOB && move->trigger[0])
        Con::executef(this, 1, "onOOBClick");

    if (mNetFlags.test(2))
    {
        delta.pos -= mPosition;
    }

    if (getControllingClient() && mContainer)
        updateContainer();

    updateRollSound(contactPct, slipAmount);
    notifyCollision();
}

void Marble::advanceCamera(const Move* move, U32 timeDelta)
{
    Point3F forwardDir = mCameraOffset;
    Point3F sideDir = mCross(mCameraOffset, -gGlobalGravityDir);
    m_point3F_normalize(sideDir);
    if (mMode == 1)
    {
		F32 dt = timeDelta;
        mCameraYaw += dt * 0.000625;
        if (mCameraPitch < 0.7)
            mCameraPitch += fminf(0.7 - mCameraPitch, dt * 0.0015625);
        else if (mCameraPitch > 0.7)
            mCameraPitch -= fminf(mCameraPitch - 0.7, dt * 0.0015625);
    }
    else
        mCameraPitch += move->pitch;
    
    mCameraYaw += move->yaw;
    if (mCameraPitch > 1.5)
        mCameraPitch = 1.5;
    if (mCameraPitch < -0.95)
        mCameraPitch = -0.95;
    
    if (mCameraYaw > 6.283185307179586)
        mCameraYaw -= 6.283185307179586;
    if (mCameraYaw < 0.0)
        mCameraYaw += 6.283185307179586;

    MatrixF pitchMat, yawMat;
    pitchMat.set(EulerF(mCameraPitch, 0, 0));
	yawMat.set(EulerF(0, 0, mCameraYaw));
    
    MatrixF gravityMat;
    if (gInterpGravityDir)
    {
        gInterpCurrentTime += timeDelta;
        if (gInterpCurrentTime < gInterpTotalTime)
        {
            F32 dt = gInterpCurrentTime;
            F32 totalT = gInterpTotalTime;
            QuatF quat;
            quat.interpolate(gStartGravityQuat, gEndGravityQuat, dt / totalT);
            quat.setMatrix(&gPrevGravityMatrix);
            gravityMat = gPrevGravityMatrix;
            gPrevGravityMatrix.getColumn(2, &gPrevGravityDir);
            gPrevGravityDir = -gPrevGravityDir;
        }
        else
        {
			gPrevGravityDir = gGlobalGravityDir;
            gPrevGravityMatrix = gGlobalGravityMatrix;
            gInterpGravityDir = false;
            gDefaultGravityTransform = gGlobalGravityMatrix;
            gravityMat = gGlobalGravityMatrix;
        }
    }
    else
        gravityMat = gPrevGravityMatrix;
    MatrixF finalMat = gravityMat;
    finalMat.mul(yawMat);
    finalMat.mul(pitchMat);
    Point3F camOff;
    finalMat.getColumn(1, &camOff);
    mCameraOffset = camOff;
}

void Marble::validateEyePoint(float dt, MatrixF* mat)
{
    if (dt != 0.0)
    {
        Point3F col1;
        mat->getColumn(1, &col1);
        Point3F pos = mObjToWorld.getPosition();
        disableCollision();
        Container* container;
        if (mNetFlags.test(2))
            container = &gClientContainer;
        else
            container = &gServerContainer;
		Point3F end = pos - col1 * mDataBlock->cameraDistance * dt;
        RayInfo rayInfo;
        U32 mask = TerrainObjectType |
            InteriorObjectType |
            WaterObjectType |
            StaticShapeObjectType |
            PlayerObjectType |
            ItemObjectType |
            VehicleObjectType;
        Point3F outPos = pos - col1 * dt;
        if (container->castRay(pos, end, mask, &rayInfo))
        {
            F32 dot = mDot(rayInfo.normal, col1);
            if (dot > 0.01)
            {
                F32 thing = mDot(pos - rayInfo.point, col1) - 0.09 / dot;
                F32 thing2 = fmaxf(0, fminf(dt, thing));
                outPos = pos - col1 * thing2;
            }
        }
        mat->setPosition(outPos);
        enableCollision();
    }
}


IMPLEMENT_CO_DATABLOCK_V1(MarbleData);

MarbleData::MarbleData()
{
    minVelocityBounceSoft = 2.5;
    minVelocityBounceHard = 12.0;
    maxRollVelocity = 25.0;
    angularAcceleration = 18.0;
    brakingAcceleration = 8.0;
    gravity = 20.0;
    staticFriction = 1.0;
    kineticFriction = 0.9;
    bounceKineticFriction = 0.2;
    maxDotSlide = 0.1;
    bounceRestitution = 0.9;
    airAcceleration = 5.0;
    energyRechargeRate = 1.0;
    jumpImpulse = 1.0;
    maxJumpTicks = 3;
    maxForceRadius = 1.0;
    cameraDistance = 2.5;
    minBounceVel = 0.1;
    minTrailSpeed = 10.0;
    minBounceSpeed = 1.0;
    bounceEmitter = 0;
    trailEmitter = 0;
    for (int i = 0; i < 6; i++)
    {
        powerUpEmitter[i] = 0;
        powerUpTime[i] = 5000;
    }
    rollHardSound = 0;
    genericShadowLevel = 0.4;
    noShadowLevel = 0.01;
}

void MarbleData::initPersistFields()
{
    addField("maxRollVelocity", TypeF32, Offset(maxRollVelocity, MarbleData));
    addField("angularAcceleration", TypeF32, Offset(angularAcceleration, MarbleData));
    addField("brakingAcceleration", TypeF32, Offset(brakingAcceleration, MarbleData));
    addField("staticFriction", TypeF32, Offset(staticFriction, MarbleData));
    addField("kineticFriction", TypeF32, Offset(kineticFriction, MarbleData));
    addField("bounceKineticFriction", TypeF32, Offset(bounceKineticFriction, MarbleData));
    addField("gravity", TypeF32, Offset(gravity, MarbleData));
    addField("maxDotSlide", TypeF32, Offset(maxDotSlide, MarbleData));
    addField("bounceRestitution", TypeF32, Offset(bounceRestitution, MarbleData));
    addField("airAcceleration", TypeF32, Offset(airAcceleration, MarbleData));
    addField("energyRechargeRate", TypeF32, Offset(energyRechargeRate, MarbleData));
    addField("jumpImpulse", TypeF32, Offset(jumpImpulse, MarbleData));
    addField("maxForceRadius", TypeF32, Offset(maxForceRadius, MarbleData));
    addField("cameraDistance", TypeF32, Offset(cameraDistance, MarbleData));
    addField("minBounceVel", TypeF32, Offset(minBounceVel, MarbleData));
    addField("minTrailSpeed", TypeF32, Offset(minTrailSpeed, MarbleData));
    addField("minBounceSpeed", TypeF32, Offset(minBounceSpeed, MarbleData));
    addField("bounceEmitter", TypeParticleEmitterDataPtr, Offset(bounceEmitter, MarbleData));
    addField("trailEmitter", TypeParticleEmitterDataPtr, Offset(trailEmitter, MarbleData));
    addField("powerupEmitter", TypeParticleEmitterDataPtr, Offset(powerUpEmitter, MarbleData), 6);
    addField("powerUpTime", TypeF32, Offset(powerUpTime, MarbleData), 6);
    addField("RollHardSound", TypeAudioProfilePtr, Offset(rollHardSound, MarbleData));
    addField("SlipSound", TypeAudioProfilePtr, Offset(slipSound, MarbleData));
    addField("Bounce1", TypeAudioProfilePtr, Offset(bounceSounds[0], MarbleData));
    addField("Bounce2", TypeAudioProfilePtr, Offset(bounceSounds[1], MarbleData));
    addField("Bounce3", TypeAudioProfilePtr, Offset(bounceSounds[2], MarbleData));
    addField("Bounce4", TypeAudioProfilePtr, Offset(bounceSounds[3], MarbleData));
    addField("JumpSound", TypeAudioProfilePtr, Offset(jumpSound, MarbleData));

    Parent::initPersistFields();
}

//----------------------------------------------------------------------------

bool MarbleData::preload(bool server, char errorBuffer[256])
{
    if (!Parent::preload(server, errorBuffer))
        return false;

    if (!server)
    {
        if (!rollHardSound && rollHardSound != 0) {
            AudioProfile* o = dynamic_cast<AudioProfile*>(Sim::findObject(rollHardSound->getId()));
			if (o != NULL)
				rollHardSound = o;
			else
				Con::errorf(ConsoleLogEntry::General, "MarbleData::preload: Invalid packet, datablock \"rollHardSound\" of class \"AudioProfile\" has bad datablockId field");
        }

        if (!slipSound && slipSound != 0) {
            AudioProfile* o = dynamic_cast<AudioProfile*>(Sim::findObject(slipSound->getId()));
            if (o != NULL)
                slipSound = o;
            else
                Con::errorf(ConsoleLogEntry::General, "MarbleData::preload: Invalid packet, datablock \"slipSound\" of class \"AudioProfile\" has bad datablockId field");
        }

        if (!jumpSound && jumpSound != 0) {
            AudioProfile* o = dynamic_cast<AudioProfile*>(Sim::findObject(jumpSound->getId()));
            if (o != NULL)
                jumpSound = o;
            else
                Con::errorf(ConsoleLogEntry::General, "MarbleData::preload: Invalid packet, datablock \"jumpSound\" of class \"AudioProfile\" has bad datablockId field");
        }

        for (int i = 0; i < 4; i++)
        {
            if (!bounceSounds[i] && bounceSounds[i] != 0) {
                AudioProfile* o = dynamic_cast<AudioProfile*>(Sim::findObject(bounceSounds[i]->getId()));
                if (o != NULL)
                    bounceSounds[i]= o;
                else
                    Con::errorf(ConsoleLogEntry::General, "MarbleData::preload: Invalid packet, datablock \"bounceSounds[i]\" of class \"AudioProfile\" has bad datablockId field");
            }
        }
    }

    return true;
}

void MarbleData::packData(BitStream* stream)
{
    stream->write(maxRollVelocity);
    stream->write(angularAcceleration);
    stream->write(brakingAcceleration);
    stream->write(gravity);
    stream->write(staticFriction);
    stream->write(kineticFriction);
    stream->write(bounceKineticFriction);
    stream->write(maxDotSlide);
    stream->write(bounceRestitution);
    stream->write(airAcceleration);
    stream->write(energyRechargeRate);
    stream->write(jumpImpulse);
    stream->write(maxForceRadius);
    stream->write(cameraDistance);
    stream->write(minBounceVel);
    stream->write(minTrailSpeed);
    stream->write(minBounceSpeed);

    if (stream->writeFlag(bounceEmitter != NULL))
        stream->writeRangedU32(bounceEmitter->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);
    if (stream->writeFlag(trailEmitter != NULL))
        stream->writeRangedU32(trailEmitter->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);
    for (int i = 0; i < 6; i++)
    {
        if (stream->writeFlag(powerUpEmitter[i] != 0))
			stream->writeRangedU32(powerUpEmitter[i]->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);
        stream->write(powerUpTime[i]);
    }

    if (stream->writeFlag(rollHardSound != NULL))
        stream->writeRangedU32(rollHardSound->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);
    
    if (stream->writeFlag(slipSound != NULL))
        stream->writeRangedU32(slipSound->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);

    if (stream->writeFlag(jumpSound != NULL))
        stream->writeRangedU32(jumpSound->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);

    for (int i = 0; i < 4; i++)
        if (stream->writeFlag(bounceSounds[i] != NULL))
            stream->writeRangedU32(bounceSounds[i]->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);

    Parent::packData(stream);
}

void MarbleData::unpackData(BitStream* stream)
{
    stream->read(&maxRollVelocity);
    stream->read(&angularAcceleration);
    stream->read(&brakingAcceleration);
    stream->read(&gravity);
    stream->read(&staticFriction);
    stream->read(&kineticFriction);
    stream->read(&bounceKineticFriction);
    stream->read(&maxDotSlide);
    stream->read(&bounceRestitution);
    stream->read(&airAcceleration);
    stream->read(&energyRechargeRate);
    stream->read(&jumpImpulse);
    stream->read(&maxForceRadius);
    stream->read(&cameraDistance);
    stream->read(&minBounceVel);
    stream->read(&minTrailSpeed);
    stream->read(&minBounceSpeed);

    if (stream->readFlag())
        Sim::findObject(stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast), bounceEmitter);
    if (stream->readFlag())
        Sim::findObject(stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast), trailEmitter);
    for (int i = 0; i < 6; i++) {
        if (stream->readFlag())
            Sim::findObject(stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast), powerUpEmitter[0]);
		stream->read(&powerUpTime[i]);
    }

	if (stream->readFlag())
		Sim::findObject(stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast), rollHardSound);
	if (stream->readFlag())
		Sim::findObject(stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast), slipSound);
	if (stream->readFlag())
		Sim::findObject(stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast), jumpSound);
	for (int i = 0; i < 4; i++)
		if (stream->readFlag())
			Sim::findObject(stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast), bounceSounds[i]);
    

    Parent::unpackData(stream);
}

//----------------------------------------------------------------------------

class MarbleUpdateEvent : public NetEvent
{
private:
    typedef NetEvent Parent;

public:
    U32 mServerMarbleId;
    U32 mPositionKey;
    U32 mPowerUpId;
    U32 mPowerUpTimer;
    Point3F mPosition;
    Vector<Marble::MaterialCollision> mCollisions;

    MarbleUpdateEvent();
    ~MarbleUpdateEvent();

    void pack(NetConnection*, BitStream*);
    void write(NetConnection*, BitStream*);
    void unpack(NetConnection*, BitStream*);
    void process(NetConnection*);

    DECLARE_CONOBJECT(MarbleUpdateEvent);
};

IMPLEMENT_CO_SERVEREVENT_V1(MarbleUpdateEvent);

MarbleUpdateEvent::MarbleUpdateEvent()
{
}

MarbleUpdateEvent::~MarbleUpdateEvent()
{
}

void MarbleUpdateEvent::pack(NetConnection* conn, BitStream* stream)
{
    stream->write(mServerMarbleId);
    stream->write(mPositionKey);
    stream->write(mPowerUpId);
    stream->write(mPowerUpTimer);
    mathWrite(*stream, mPosition);
    stream->write(mCollisions.size());
    for (int i = 0; i < mCollisions.size(); i++)
    {
		stream->write(mCollisions[i].ghostIndex);
		stream->write(mCollisions[i].materialId);
    }
}

void MarbleUpdateEvent::write(NetConnection* conn, BitStream* stream)
{
    pack(conn, stream);
}

void MarbleUpdateEvent::unpack(NetConnection* conn, BitStream* stream)
{
	stream->read(&mServerMarbleId);
	stream->read(&mPositionKey);
	stream->read(&mPowerUpId);
	stream->read(&mPowerUpTimer);
	mathRead(*stream, &mPosition);
	U32 size;
	stream->read(&size);
    mCollisions.increment(size);
	for (int i = 0; i < size; i++)
	{
		stream->read(&mCollisions[i].ghostIndex);
		stream->read(&mCollisions[i].materialId);
        mCollisions[i].ghostObject = conn->resolveObjectFromGhostIndex(mCollisions[i].ghostIndex);
	}
}

void MarbleUpdateEvent::process(NetConnection* conn)
{
    GameConnection* gc = (GameConnection*)conn;
    if (gc->getControlObject())
    {
        Marble* marble = dynamic_cast<Marble*>(gc->getControlObject());
        if (marble && marble->getId() == mServerMarbleId)
        {
            marble->clientStateUpdated(mPosition, mPositionKey, mPowerUpId, mPowerUpTimer, mCollisions);
        }
    }
}

void Marble::controlPrePacketSend(GameConnection* conn)
{
    MarbleUpdateEvent* mue = new MarbleUpdateEvent;
    mue->mServerMarbleId = mServerMarbleId;
    mue->mPosition = mPosition;
    mue->mPositionKey = mPositionKey;
    mue->mPowerUpId = mPowerUpId;
    mue->mPowerUpTimer = mPowerUpTimer;
    mue->mCollisions = mMaterialCollisions;
    conn->postNetEvent(mue);
    mMaterialCollisions.clear();
}

ConsoleFunction(setGravityDir, void, 2, 3, "(gravity, snap)")
{
    Point3F xvec;
    Point3F yvec;
    Point3F zvec;
    dSscanf(argv[1], "%g %g %g %g %g %g %g %g %g", &xvec.x, &xvec.y, &xvec.z, &yvec.x, &yvec.y, &yvec.z, &zvec.x, &zvec.y, &zvec.z);

    gGlobalGravityDir = zvec;

    MatrixF oldMat = gGlobalGravityMatrix;
    gGlobalGravityMatrix.setColumn(0, xvec);
	gGlobalGravityMatrix.setColumn(1, -yvec);
	gGlobalGravityMatrix.setColumn(2, -zvec);
    
    Point3F up = -zvec;
    
    if (argc == 3 && dAtob(argv[2]))
    {
        gPrevGravityDir = gGlobalGravityDir;
        gPrevGravityMatrix = gGlobalGravityMatrix;
        gDefaultGravityTransform = gGlobalGravityMatrix;
        gInterpGravityDir = false;
        return;
    }
   
    Point3F oldUp;
    Point3D rot;
    oldMat.getColumn(2, &oldUp);
    mCross(oldUp, up, &rot);

    float len = rot.len();
    if (len > 0.1)
    {
        if (len > 1.0f)
            len = 1.0f;
        if (len < -1.0f)
            len = -1.0f;
       m_point3D_normalize_f(rot, asinf(len));
       gGlobalGravityMatrix = oldMat;
       rotateMatrix(gGlobalGravityMatrix, rot);
       gGlobalGravityMatrix.setColumn(2, up);
    }
    else
    {
        Point3F oldX;;
        oldMat.getColumn(0, &oldX);
        gGlobalGravityMatrix.setColumn(0, oldX);
        gGlobalGravityMatrix.setColumn(1, mCross(up, oldX));
    }
    if (!gInterpGravityDir)
    {
        gPrevGravityMatrix = oldMat;
    }
    gStartGravityQuat.set(gPrevGravityMatrix);
	gEndGravityQuat.set(gGlobalGravityMatrix);
    gInterpCurrentTime = 0;
    gInterpTotalTime = 325;
    gInterpGravityDir = true;
}

ConsoleFunction(getGravityDir, const char*, 1, 1, "()")
{
    char* retBuf = Con::getReturnBuffer(256);
    dSprintf(retBuf, 256, "%g %g %g", gGlobalGravityDir.x, gGlobalGravityDir.y, gGlobalGravityDir.z);

    return retBuf;
}


ConsoleMethod(Marble, setOOB, void, 3, 3, "(oob)")
{
    object->setOOB(dAtob(argv[2]));
}

ConsoleMethod(Marble, getPosition, const char*, 2, 2, "()")
{
    static char buffer[100];
    Point3F pos = object->getPosition();
    dSprintf(buffer, 100, "%f %f %f", pos.x, pos.y, pos.z);

    return buffer;
}

ConsoleMethod(Marble, doPowerUp, void, 3, 3, "(powerup)")
{
    F32 id = dAtof(argv[2]);
    Con::printf("Did powerup! - %g", id);
    object->doPowerUp(id);
}

ConsoleMethod(Marble, setPowerUpId, void, 3, 4, "(id)")
{
    bool reset = false;
    if (argc > 3)
        reset = dAtob(argv[3]);

    object->setPowerUpId(dAtoi(argv[2]), reset);
}

ConsoleMethod(Marble, getPad, S32, 2, 2, "()")
{
    SceneObject* pad = object->getPad();
    if (pad)
        return pad->getId();

    return 0;
}

ConsoleMethod(Marble, setMode, void, 3, 3, "(mode)")
{
    char* modeList[3];
    modeList[0] = "Normal";
    modeList[1] = "Victory";
    modeList[2] = "Start";
    for (int i = 0; i < 3; i++)
    {
        if (dStricmp(modeList[i], argv[2]) == 0)
        {
            object->setMode(i);
            return;
        }
    }
    Con::printf("Marble:: Unkonwn marble mode: %s", argv[2]);
    object->setMode(0);
}

ConsoleMethod(Marble, setPad, void, 3, 3, "(pad)")
{
    U32 padId = dAtoi(argv[2]);
    if (!padId)
    {
        object->setPad(0);
        return;
    }

    SceneObject* pad;
    if (Sim::findObject(padId, pad))
        object->setPad(pad);
    else
        Con::errorf("Marble::setPad: Not a SceneObject");
}


ConsoleMethod(Marble, setPosition, void, 4, 4, "(transform, mouseY)")
{
    Point3F posf;
    AngAxisF angAxis;

    // Without this, if an axis is not provided, these are NaN
    angAxis.axis.set(0, 0, 0);
    angAxis.angle = 0;

    dSscanf(argv[2], "%f %f %f %f %f %f %f", &posf.x, &posf.y, &posf.z, &angAxis.axis.x, &angAxis.axis.y, &angAxis.axis.z, &angAxis.angle);

    object->setPosition(Point3D(posf.x, posf.y, posf.z), angAxis, dAtof(argv[3]));
}