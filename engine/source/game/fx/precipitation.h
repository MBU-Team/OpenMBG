//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _H_precipitation
#define _H_precipitation

#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
#endif
#ifndef _TSSHAPE_H_
#include "ts/tsShape.h"
#endif
#ifndef _AUDIODATABLOCK_H_
#include "audio/audioDataBlock.h"
#endif


#define MAX_NUM_DROPS 2000
#define MAX_NUM_COLOR 3
#define NUM_TEXTURES 16
#define MIN_ZRADIUS 0.01f 

class AudioProfile;

//--------------------------------------------------------------------------
class PrecipitationData : public GameBaseData {
   typedef GameBaseData Parent;

  public:
   AudioProfile* soundProfile;
   S32           soundProfileId;
   S32           mType;
   F32           mMaxSize;
	F32           mSizeX;
   F32           mSizeY;
   StringTableEntry mMaterialListName;
   
 /////////////////////////  
 //JohnA Will remove  
 // Used to tweak the precipitation
 /////////////////////////  
   F32 tMoveingBoxPer;      
   F32 tDivHeightVal;
   F32 tSizeBigBox;
   F32 tTopBoxSpeed;
   F32 tFrontBoxSpeed;
   F32 tTopBoxDrawPer;
   F32 tBottomDrawHeight;
   F32 tSkipIfPer;
   F32 tBottomSpeedPer;
   F32 tFrontSpeedPer;
   F32 tFrontRadiusPer;
 /////////////////////////  

   PrecipitationData();
   DECLARE_CONOBJECT(PrecipitationData);
   bool onAdd();
   static void  initPersistFields();
   virtual void packData(BitStream* stream);
   virtual void unpackData(BitStream* stream);
};


typedef struct
{
   F32 speed;
	Point3F curPos;
	F32 startTime;
	F32 endTime;
	Point3F startPos;
	Point3F sPosOffset;
   F32 size;
   Point3F offset;
   S32 colorIndex;
   S32 texIndex;
   ColorF colorOffset;
   F32 rotation;
   bool stillFalling;
   F32 startDiff;
   S32 dropType;
   bool notValid;
}FallingInfo;
   
//--------------------------------------------------------------------------
enum State
{
   done = 0,
   in = 1,
   out = 2
};

typedef struct
{
   F32 lastTime;
   F32 endPercentage;
   F32 time;
   F32 speed;
   S32 numDrops;
   State state;
   F32 currentTime;
}PrecipStormData;

class Precipitation : public GameBase
{
  private:
   typedef GameBase Parent;
   PrecipitationData*   mDataBlock;

   FallingInfo mFallingObj[MAX_NUM_DROPS];	
   Point2F mTexCoord[NUM_TEXTURES];
   F32 mCurrentTime;
   F32 mLastRenderTime;
   Point3F mRotX, mRotZ;
   Point2F mOffsetVal;
   Point3F mNewCenter;
   Point3F mLastPos;
   bool mRandomHeight;
   TextureHandle mTextures[20];          
	MaterialList   mMaterialList;
   S32 mNumTextures;      
   bool mFirstTime;
   AUDIOHANDLE mAudioHandle;
   F32 mPercentage;
   S32 mNumDrops;
   PrecipStormData mStormData;
   bool mStormPrecipitationOn;
   Box3F mBox;
   F32 mAverageSpeed;
   F32 mLastHeight;
   Point3F mOffset;
   Point3F mShiftPrecip;
   S32           mColorCount;
   ColorF        mColor[MAX_NUM_COLOR];
   F32           mMinVelocity;
   F32           mMaxVelocity;
   S32           mMaxDrops;
   S32           mRadius;
   F32           mOffsetSpeed;
   
   void processTick(const Move*);
  protected:
   bool onAdd();
   void onRemove();

   void advanceTime(F32 dt);

   // Rendering
   bool prepRenderImage(SceneState*, const U32, const U32, const bool);
   void renderObject(SceneState*, SceneRenderImage*);
   void renderPrecip(Point3F camPos);
   void renderSand();
   void calcVelocityBox(Point3F camPos, F32 &curZVelocity, F32 &renderPer);
   void loadDml();
   void setupTexCoords();
   F32  getRandomVal();
  public:
   enum NetMaskBits {
      InitMask = BIT(0),
      PercentageMask = BIT(1),
      StormMask = BIT(2),
      StormShowMask = BIT(3)
   };

   Precipitation();
   ~Precipitation();
   void inspectPostApply();
   void setInitialState(const Point3F& point, const Point3F& normal, const F32 fade = 1.0);
   void setPercentage(F32);

   bool onNewDataBlock(GameBaseData* dptr);
   DECLARE_CONOBJECT(Precipitation);
   static void initPersistFields();
   static void consoleInit();
   
   static bool smPrecipitationOn;
   static bool smPrecipitationPause;
   
   void setDefaultValues();
   void stormShow(bool show);
   void setupStorm(F32 percentage, F32 time);
   void startStorm();
   void updateStorm();
   
   U32  packUpdate(NetConnection*, U32 mask, BitStream* stream);
   void unpackUpdate(NetConnection*, BitStream* stream);
};

#endif // _H_precipitation

