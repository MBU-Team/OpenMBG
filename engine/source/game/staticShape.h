//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _STATICSHAPE_H_
#define _STATICSHAPE_H_

#ifndef _SHAPEBASE_H_
#include "game/shapeBase.h"
#endif

//----------------------------------------------------------------------------

struct StaticShapeData: public ShapeBaseData {
   typedef ShapeBaseData Parent;

  public:
   StaticShapeData();

   bool  noIndividualDamage;
   S32   dynamicTypeField;
   bool  isShielded;
   F32   energyPerDamagePoint;

   //
   DECLARE_CONOBJECT(StaticShapeData);
   static void initPersistFields();
   virtual void packData(BitStream* stream);
   virtual void unpackData(BitStream* stream);
};


//----------------------------------------------------------------------------

class StaticShape: public ShapeBase
{
   typedef ShapeBase Parent;

   StaticShapeData*  mDataBlock;
   bool              mPowered;

   void onUnmount(ShapeBase* obj,S32 node);

  protected:
   enum MaskBits {
      PositionMask = Parent::NextFreeMask,
      NextFreeMask = Parent::NextFreeMask << 1
   };

public:
   DECLARE_CONOBJECT(StaticShape);

   StaticShape();
   ~StaticShape();

   bool onAdd();
   void onRemove();
   bool onNewDataBlock(GameBaseData* dptr);

   void processTick(const Move *move);
   void interpolateTick(F32 delta);
   void setTransform(const MatrixF &mat);

   U32  packUpdate  (NetConnection *conn, U32 mask, BitStream *stream);
   void unpackUpdate(NetConnection *conn,           BitStream *stream);
   
   // power
   void setPowered(bool power)      {mPowered = power;}
   bool isPowered()                 {return(mPowered);}
};


#endif
