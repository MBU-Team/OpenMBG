//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "core/dnet.h"
#include "console/consoleTypes.h"
#include "console/simBase.h"
#include "game/gameConnection.h"
#include "game/shapeBase.h"
#include "core/bitStream.h"
#include "sim/pathManager.h"
#include "audio/audio.h"
#include "game/game.h"
#include "sceneGraph/sceneGraph.h"
#include "game/gameConnectionEvents.h"

F32 MoveManager::mForwardAction = 0;
F32 MoveManager::mBackwardAction = 0;
F32 MoveManager::mUpAction = 0;
F32 MoveManager::mDownAction = 0;
F32 MoveManager::mLeftAction = 0;
F32 MoveManager::mRightAction = 0;
   
bool MoveManager::mFreeLook = false;
F32 MoveManager::mPitch = 0;
F32 MoveManager::mYaw = 0;
F32 MoveManager::mRoll = 0;

F32 MoveManager::mPitchUpSpeed = 0;
F32 MoveManager::mPitchDownSpeed = 0;
F32 MoveManager::mYawLeftSpeed = 0;
F32 MoveManager::mYawRightSpeed = 0;
F32 MoveManager::mRollLeftSpeed = 0;
F32 MoveManager::mRollRightSpeed = 0;

U32 MoveManager::mTriggerCount[MaxTriggerKeys] = { 0, };
U32 MoveManager::mPrevTriggerCount[MaxTriggerKeys] = { 0, };

#define MAX_MOVE_PACKET_SENDS 4

const Move NullMove =
{
   16,16,16,
   0,0,0,
   0,0,0,   // x,y,z
   0,0,0,   // Yaw, pitch, roll,
   0,0,

   false,
   false,false,false,false,false,false
};

void MoveManager::init()
{
   Con::addVariable("mvForwardAction", TypeF32, &mForwardAction);
   Con::addVariable("mvBackwardAction", TypeF32, &mBackwardAction);
   Con::addVariable("mvUpAction", TypeF32, &mUpAction);
   Con::addVariable("mvDownAction", TypeF32, &mDownAction);
   Con::addVariable("mvLeftAction", TypeF32, &mLeftAction);
   Con::addVariable("mvRightAction", TypeF32, &mRightAction);
   
   Con::addVariable("mvFreeLook", TypeBool, &mFreeLook);
   Con::addVariable("mvPitch", TypeF32, &mPitch);
   Con::addVariable("mvYaw", TypeF32, &mYaw);
   Con::addVariable("mvRoll", TypeF32, &mRoll);
   Con::addVariable("mvPitchUpSpeed", TypeF32, &mPitchUpSpeed);
   Con::addVariable("mvPitchDownSpeed", TypeF32, &mPitchDownSpeed);
   Con::addVariable("mvYawLeftSpeed", TypeF32, &mYawLeftSpeed);
   Con::addVariable("mvYawRightSpeed", TypeF32, &mYawRightSpeed);
   Con::addVariable("mvRollLeftSpeed", TypeF32, &mRollLeftSpeed);
   Con::addVariable("mvRollRightSpeed", TypeF32, &mRollRightSpeed);

   for(U32 i = 0; i < MaxTriggerKeys; i++)
   {
      char varName[256];
      dSprintf(varName, sizeof(varName), "mvTriggerCount%d", i);
      Con::addVariable(varName, TypeS32, &mTriggerCount[i]);
   }
}

static inline F32 clampFloatWrap(F32 val)
{
   return val - F32(S32(val));
}

static F32 clampFloatClamp(F32 val, U32 bits)
{
   if(val < 0)
      val = 0;
   else if(val > 1)
      val = 1;
   F32 mask = (1 << bits);
   return U32(val * mask) / F32(mask);
}

static inline S32 clampRangeClamp(F32 val)
{
   if(val < -1)
      return 0;
   if(val > 1)
      return 32;
   return (S32)((val + 1) * 16);
}


#define FANG2IANG(x)   ((U32)((S16)((F32(0x10000) / M_2PI) * x)) & 0xFFFF)
#define IANG2FANG(x)   (F32)((M_2PI / F32(0x10000)) * (F32)((S16)x))

void Move::unclamp()
{
   yaw = IANG2FANG(pyaw);
   pitch = IANG2FANG(ppitch);
   roll = IANG2FANG(proll);

   x = (px - 16) / F32(16);
   y = (py - 16) / F32(16);
   z = (pz - 16) / F32(16);
}

void Move::clamp()
{
   // angles are all 16 bit.
   pyaw = FANG2IANG(yaw);
   ppitch = FANG2IANG(pitch);
   proll = FANG2IANG(roll);

   px = clampRangeClamp(x);
   py = clampRangeClamp(y);
   pz = clampRangeClamp(z);
   unclamp();
}

void Move::pack(BitStream *stream)
{
   if(stream->writeFlag(pyaw != 0))
      stream->writeInt(pyaw, 16);
   if(stream->writeFlag(ppitch != 0))
      stream->writeInt(ppitch, 16);
   if(stream->writeFlag(proll != 0))
      stream->writeInt(proll, 16);

   stream->writeInt(px, 6);
   stream->writeInt(py, 6);
   stream->writeInt(pz, 6);
   stream->writeFlag(freeLook);

   for(U32 i = 0; i < MaxTriggerKeys; i++)
      stream->writeFlag(trigger[i]);
}

void Move::unpack(BitStream *stream)
{
   if(stream->readFlag())
      pyaw = stream->readInt(16);
   else
      pyaw = 0;
   if(stream->readFlag())
      ppitch = stream->readInt(16);
   else
      ppitch = 0;
   if(stream->readFlag())
      proll = stream->readInt(16);
   else
      proll = 0;
   px = stream->readInt(6);
   py = stream->readInt(6);
   pz = stream->readInt(6);

   unclamp();
   freeLook = stream->readFlag();

   for(U32 i = 0; i < MaxTriggerKeys; i++)
      trigger[i] = stream->readFlag();
}

bool GameConnection::getNextMove(Move &curMove)
{
   if(mMoveList.size() > MaxMoveQueueSize)
      return false;
      
   F32 pitchAdd = MoveManager::mPitchUpSpeed - MoveManager::mPitchDownSpeed;
   F32 yawAdd = MoveManager::mYawLeftSpeed - MoveManager::mYawRightSpeed;
   F32 rollAdd = MoveManager::mRollRightSpeed - MoveManager::mRollLeftSpeed;

   curMove.pitch = MoveManager::mPitch + pitchAdd;
   curMove.yaw = MoveManager::mYaw + yawAdd;
   curMove.roll = MoveManager::mRoll + rollAdd;
   
   MoveManager::mPitch = 0;
   MoveManager::mYaw = 0;
   MoveManager::mRoll = 0;

   curMove.x = MoveManager::mRightAction - MoveManager::mLeftAction;
   curMove.y = MoveManager::mForwardAction - MoveManager::mBackwardAction;
   curMove.z = MoveManager::mUpAction - MoveManager::mDownAction;
   
   curMove.freeLook = MoveManager::mFreeLook;

   for(U32 i = 0; i < MaxTriggerKeys; i++)
   {
      curMove.trigger[i] = false;
      if(MoveManager::mTriggerCount[i] & 1)
         curMove.trigger[i] = true;
      else if(!(MoveManager::mPrevTriggerCount[i] & 1) && MoveManager::mPrevTriggerCount[i] != MoveManager::mTriggerCount[i])
         curMove.trigger[i] = true;
      MoveManager::mPrevTriggerCount[i] = MoveManager::mTriggerCount[i];
   }
   curMove.clamp();  // clamp for net traffic
   return true;
}

void GameConnection::pushMove(const Move &mv)
{
   U32 id = mFirstMoveIndex + mMoveList.size();
   U32 sz = mMoveList.size();
   mMoveList.push_back(mv);
   mMoveList[sz].id = id;
   mMoveList[sz].sendCount = 0;
}

void GameConnection::getMoveList(Move** movePtr,U32* numMoves)
{
   if (isServerConnection())
   {
      // give back moves starting at the last client move...
      
      AssertFatal(mLastClientMove >= mFirstMoveIndex, "Bad move request");
      AssertFatal(mLastClientMove - mFirstMoveIndex <= mMoveList.size(), "Desynched first and last move.");
      *numMoves = mMoveList.size() - mLastClientMove + mFirstMoveIndex;
      *movePtr = mMoveList.address() + mLastClientMove - mFirstMoveIndex;
   }
   else
   {
      // On the server we keep our own move list.
      *numMoves = (mMoveList.size() < mMoveCredit)?
         mMoveList.size(): mMoveCredit;
      *movePtr = mMoveList.begin();
      // 
      mMoveCredit -= *numMoves;
      mMoveList.setSize(*numMoves);
   }
}

void GameConnection::collectMove(U32 time)
{
   Move mv;
   if(!isPlayingBack() && getNextMove(mv))
   {
      pushMove(mv);
      recordBlock(BlockTypeMove, sizeof(Move), &mv);
   }
}

void GameConnection::clearMoves(U32 count)
{
   if (isServerConnection()) {
      mLastClientMove += count;
   }
   else {
      AssertFatal(count <= mMoveList.size(),"GameConnection: Clearing too many moves");
      if (count == mMoveList.size())
         mMoveList.clear();
      else
         while (count--)
            mMoveList.pop_front();
   }
}

void GameConnection::incMoveCredit(U32 ticks)
{
   AssertFatal(!isServerConnection(), "Cannot inc move credit on the client.");
   // Game tick increment
   if ((mMoveCredit += ticks) > MaxMoveCount)
      mMoveCredit = MaxMoveCount;

   // Clear pending moves for the elapsed time if there
   // is no control object.
   if (mControlObject.isNull())
      mMoveList.clear();
}

bool GameConnection::areMovesPending()
{
   return isServerConnection() ?
      mMoveList.size() - mLastClientMove + mFirstMoveIndex :
      mMoveList.size();
}

bool GameConnection::isBacklogged()
{
   // If there are no pending moves and the input queue is full,
   // then the connection to the server must be clogged.
   if(!isServerConnection())
      return false;
   return mLastClientMove - mFirstMoveIndex == mMoveList.size() &&
      mMoveList.size() >= MaxMoveCount;
}


void GameConnection::moveWritePacket(BitStream *bstream)
{
   Move* move;
   U32 count;
   AssertFatal(mLastMoveAck == mFirstMoveIndex, "Invalid move index.");
   count = mMoveList.size();
   move = mMoveList.address();
   U32 start = mLastMoveAck;
   U32 offset;
   for(offset = 0; offset < count; offset++)
      if(move[offset].sendCount < MAX_MOVE_PACKET_SENDS)
         break;
   if(offset == count && count != 0)
      offset--;

   start += offset;
   count -= offset;
   
   if (count > MaxMoveCount)
      count = MaxMoveCount;
   bstream->writeInt(start,32);
   bstream->writeInt(count,MoveCountBits);
   for (int i = 0; i < count; i++)
   {
      move[offset + i].sendCount++;
      move[offset + i].pack(bstream);
   }
}

void GameConnection::moveReadPacket(BitStream *bstream)
{
   // Server side packet read.
   U32 start = bstream->readInt(32);
   U32 count = bstream->readInt(MoveCountBits);

   // Skip forward (must be starting up), or over the moves
   // we already have.
   int skip = mLastMoveAck - start;
   if (skip < 0) {
      mLastMoveAck = start;
      //mMoveList.clear();
   }
   else {
      Move tmp;
      if (skip > count)
         skip = count;
      for (int i = 0; i < skip; i++)
         tmp.unpack(bstream);
      start += skip;
      count = count - skip;
   }

   // Put the rest on the move list.
   int index = mMoveList.size();
   mMoveList.increment(count);
   while (index < mMoveList.size())
   {
      mMoveList[index].unpack(bstream);
      mMoveList[index].id = start++;
      index ++;
   }

   mLastMoveAck += count;
}


