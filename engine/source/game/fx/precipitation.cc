//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "game/fx/precipitation.h"

#include "dgl/dgl.h"
#include "math/mathIO.h"
#include "console/consoleTypes.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sceneState.h"
#include "terrain/sky.h"
#include "game/gameConnection.h"
#include "game/player.h"

#define COLOR_OFFSET 0.25

bool Precipitation::smPrecipitationOn = true;
bool Precipitation::smPrecipitationPause = false;

IMPLEMENT_CO_NETOBJECT_V1(Precipitation);
IMPLEMENT_CO_DATABLOCK_V1(PrecipitationData);

namespace {

MRandomLCG sgRandom(0xdeadbeef);


} // namespace {}

//----------------------------------------------------------------------------
//--------------------------------------


PrecipitationData::PrecipitationData()
{
   soundProfile      = NULL;
   soundProfileId    = 0;
   mType             = 0;
   mMaxSize          = 1.0f;
   mMaterialListName = NULL;
   mSizeX             = 1.0;
   mSizeY             = 1.0;
}

IMPLEMENT_GETDATATYPE(PrecipitationData)
   IMPLEMENT_SETDATATYPE(PrecipitationData)

   void PrecipitationData::initPersistFields()
{
   Parent::initPersistFields();

   Con::registerType("GameBaseDataPtr", TypeGameBaseDataPtr, sizeof(PrecipitationData*),
                     REF_GETDATATYPE(PrecipitationData),
                     REF_SETDATATYPE(PrecipitationData));
   
   addField("soundProfile", TypeAudioProfilePtr, Offset(soundProfile, PrecipitationData));
   addField("type",         TypeS32,    Offset(mType, PrecipitationData));
   addField("maxSize",      TypeF32,    Offset(mMaxSize, PrecipitationData));
   addField("materialList", TypeFilename, Offset(mMaterialListName,PrecipitationData));
   addField("sizeX",        TypeF32,    Offset(mSizeX, PrecipitationData));
   addField("sizeY",        TypeF32,    Offset(mSizeY, PrecipitationData));

 /////////////////////////  
 //JohnA Will remove  
 // Used to tweak the precipitation
 /////////////////////////  
   addField("movingBoxPer",        TypeF32,    Offset(tMoveingBoxPer, PrecipitationData));
   addField("divHeightVal",        TypeF32,    Offset(tDivHeightVal, PrecipitationData));
   addField("sizeBigBox",        TypeF32,    Offset(tSizeBigBox, PrecipitationData));
   addField("topBoxSpeed",        TypeF32,    Offset(tTopBoxSpeed, PrecipitationData));
   addField("frontBoxSpeed",        TypeF32,    Offset(tFrontBoxSpeed, PrecipitationData));
   addField("topBoxDrawPer",        TypeF32,    Offset(tTopBoxDrawPer, PrecipitationData));
   addField("bottomDrawHeight",        TypeF32,    Offset(tBottomDrawHeight, PrecipitationData));
   addField("skipIfPer",        TypeF32,    Offset(tSkipIfPer, PrecipitationData));
   addField("bottomSpeedPer",        TypeF32,    Offset(tBottomSpeedPer, PrecipitationData));
   addField("frontSpeedPer",        TypeF32,    Offset(tFrontSpeedPer, PrecipitationData));
   addField("frontRadiusPer",        TypeF32,    Offset(tFrontRadiusPer, PrecipitationData));
 /////////////////////////  

}

bool PrecipitationData::onAdd()
{
   if (Parent::onAdd() == false)
      return false;

   if (!soundProfile && soundProfileId != 0)
      if (Sim::findObject(soundProfileId, soundProfile) == false)
         Con::errorf(ConsoleLogEntry::General, "Error, unable to load sound profile for precipitation datablock");

   if (mSizeX <= 0.0f || mSizeX > 20.0f) {
      Con::warnf(ConsoleLogEntry::General, "PrecipitationData(%s)::onAdd: sizeX must be in the range [0 >, 20]", getName());
      mSizeX = 1.0;
   }

   if (mSizeY <= 0.0f || mSizeY > 20.0f) {
      Con::warnf(ConsoleLogEntry::General, "PrecipitationData(%s)::onAdd: sizeY must be in the range [0 >, 20]", getName());
      mSizeY = 1.0;
   }
 
   return true;
}

void PrecipitationData::packData(BitStream* stream)
{
   Parent::packData(stream);

   if (stream->writeFlag(soundProfile != NULL))
      stream->writeRangedU32(soundProfile->getId(), DataBlockObjectIdFirst,
                             DataBlockObjectIdLast);
   stream->write(mType);
   stream->write(mMaxSize);
   stream->writeString(mMaterialListName);
   stream->write(mSizeX);
   stream->write(mSizeY);

 /////////////////////////  
 //JohnA Will remove  
 // Used to tweak the precipitation
 /////////////////////////  
   stream->write(tMoveingBoxPer);
   stream->write(tDivHeightVal);
   stream->write(tSizeBigBox);
   stream->write(tTopBoxSpeed);
   stream->write(tFrontBoxSpeed);
   stream->write(tTopBoxDrawPer);
   stream->write(tBottomDrawHeight);
   stream->write(tSkipIfPer);
   stream->write(tBottomSpeedPer);
   stream->write(tFrontSpeedPer);
   stream->write(tFrontRadiusPer);
 /////////////////////////  

}

void PrecipitationData::unpackData(BitStream* stream)
{
   Parent::unpackData(stream);

   if (stream->readFlag())
      soundProfileId = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
   else
      soundProfileId = 0;

   stream->read(&mType);
   stream->read(&mMaxSize);
   mMaterialListName = stream->readSTString();
   stream->read(&mSizeX);
   stream->read(&mSizeY);

 /////////////////////////  
 //JohnA Will remove  
 // Used to tweak the precipitation
 /////////////////////////  
   stream->read(&tMoveingBoxPer);
   stream->read(&tDivHeightVal);
   stream->read(&tSizeBigBox);
   stream->read(&tTopBoxSpeed);
   stream->read(&tFrontBoxSpeed);
   stream->read(&tTopBoxDrawPer);
   stream->read(&tBottomDrawHeight);
   stream->read(&tSkipIfPer);
   stream->read(&tBottomSpeedPer);
   stream->read(&tFrontSpeedPer);
   stream->read(&tFrontRadiusPer);
 /////////////////////////  
}

//--------------------------------------------------------------------------
//--------------------------------------

Precipitation::Precipitation()
{
   mTypeMask |= ProjectileObjectType;

   mNetFlags.set(ScopeAlways);
   mNumDrops = 0;
   mCurrentTime = 0;
   mAudioHandle = 0;
   mPercentage = 1.0f;
   mFirstTime = true;
   mStormData.lastTime = 0.0f;
   mStormData.endPercentage = -1.0f;
   mStormData.time = 0.0f;
   mStormData.speed = 0.0f;
   mStormData.state = done;
   mStormData.numDrops = 0;
   mStormData.currentTime = 0.0f;
   mStormPrecipitationOn = true;
   mLastPos.set(0.0f, 0.0f, 0.0f);
   mRandomHeight = false;
   mAverageSpeed = 0.0f;
   mLastHeight = 0.0f;
   mShiftPrecip.set(0.0f, 0.0f, 0.0f);
   mColorCount       = 0;
   for(S32 x = 0; x < MAX_NUM_COLOR; ++x)
      mColor[x].set(-1.0f, 0.0f, 0.0f);

   mMinVelocity = -1.0f;
   mMaxVelocity = -1.0f;
   mOffset.set(0.0f, 0.0f, 1.0f);
   mOffsetSpeed = 0.25f;
   mMaxDrops         = -1;
   mRadius           = -1;
}

Precipitation::~Precipitation()
{

}

void Precipitation::inspectPostApply()
{
   setMaskBits(InitMask);
}

//--------------------------------------------------------------------------
void Precipitation::initPersistFields()
{
   Parent::initPersistFields();

   addGroup("Velocity");	// MM: Added Group Header.
   addField("offsetSpeed",  TypeF32,    Offset(mOffsetSpeed, Precipitation));
   addField("minVelocity",  TypeF32,    Offset(mMinVelocity, Precipitation));
   addField("maxVelocity",  TypeF32,    Offset(mMaxVelocity, Precipitation));
   endGroup("Velocity");	// MM: Added Group Footer.

   addGroup("Colors");	// MM: Added Group Header.
   addField("color1",       TypeColorF, Offset(mColor[0], Precipitation));
   addField("color2",       TypeColorF, Offset(mColor[1], Precipitation));
   addField("color3",       TypeColorF, Offset(mColor[2], Precipitation));
   endGroup("Colors");	// MM: Added Group Footer.

   addGroup("Misc");	// MM: Added Group Header.
   addField("percentage",   TypeF32, Offset(mPercentage, Precipitation));
   addField("maxNumDrops",  TypeS32,    Offset(mMaxDrops, Precipitation));
   addField("maxRadius",    TypeS32,    Offset(mRadius, Precipitation));
   endGroup("Misc");	// MM: Added Group Footer.
}

ConsoleMethod(Precipitation, setPercentange, void, 3, 3, "precipitation.setPercentage(percentage <1.0 to 0.0>)")
{
   object->setPercentage(dAtof(argv[2]));
}

ConsoleMethod(Precipitation, stormPrecipitation, void, 4, 4, "precipitation.stormPrecipitation(Percentage <0 to 1>, Time<sec>)")
{
   object->setupStorm(dAtof(argv[2]), dAtof(argv[3]));
}

ConsoleMethod(Precipitation, stormShow, void, 3, 3, "precipitation.stormShow(bool)")
{
   object->stormShow(dAtob(argv[2]));
}

void Precipitation::consoleInit()
{
   Con::addVariable("$pref::precipitationOn", TypeBool, &smPrecipitationOn);
   Con::addVariable("$pref::prePause", TypeBool, &smPrecipitationPause);
}

//--------------------------------------------------------------------------
bool Precipitation::onAdd()
{
   if(!Parent::onAdd())
      return false;

   if (mPercentage > 1.0f || mPercentage < 0.0f) {
      Con::warnf(ConsoleLogEntry::General, "Precipitation::onAdd - Percentage is invalid. <= 1.0 or >= 0.0 ");
      mPercentage = 1.0f;
   }
       
   if (isClientObject()) {
      mRandomHeight = true;
      for (U32 i = 0; i < mMaxDrops; i++) 
         mFallingObj[i].notValid = true;

      if(mDataBlock->mMaterialListName[0])
         loadDml();

      if (mDataBlock->soundProfile && smPrecipitationOn)
         mAudioHandle = alxPlay(mDataBlock->soundProfile, &getTransform() );
      mNumDrops = (S32)(mMaxDrops * mPercentage);
      mStormData.numDrops = mNumDrops;
      setupTexCoords();
   }
   else
      assignName("Precipitation");
      
   mObjBox.min.set(-1e6, -1e6, -1e6);
   mObjBox.max.set( 1e6,  1e6,  1e6);

   resetWorldBox();
   addToScene();
   setDefaultValues();

   return true;
}

void Precipitation::setDefaultValues()
{
   mColorCount = 0;
   for(S32 x = 0; x < MAX_NUM_COLOR; ++x)
      if(mColor[x].red >= 0.0f)
         mColorCount++;
      else
         break;   
   if(mColorCount == 0)
   {
      if(mDataBlock->mType == 0)
         mColor[0].set(0.6, 0.6, 0.6);   
      else if(mDataBlock->mType == 1)
         mColor[0].set(1.0, 1.0, 1.0);   
      else
         mColor[0].set(0.9, 0.8, 0.5);   
       mColorCount = 1;
   }
   if(mMinVelocity == -1.0f)
   {
      if(mDataBlock->mType == 0)
         mMinVelocity = 1.25f;
      else if(mDataBlock->mType == 1)
         mMinVelocity = 0.25f;
      else
         mMinVelocity = 0.25f;
   }
   if(mMaxVelocity == -1.0f)
   {
      if(mDataBlock->mType == 0)
         mMaxVelocity = 4.0f;
      else if(mDataBlock->mType == 1)
         mMaxVelocity = 1.5f; 
      else
         mMaxVelocity = 1.0f;
   }

   if(mRadius == -1.0f)
   {
      if(mDataBlock->mType == 0)
         mRadius = 80;
      else if(mDataBlock->mType == 1)
         mRadius = 125;
      else
         mRadius = 80;
   }
   
   if(mMaxDrops > MAX_NUM_DROPS || mMaxDrops < 0)
      mMaxDrops = MAX_NUM_DROPS;
}

U32 Precipitation::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
   Parent::packUpdate(con, mask, stream);

   if(stream->writeFlag(mask & InitMask))
   {
      stream->write(mPercentage);
      stream->write(mColorCount);
      for(S32 x = 0; x < mColorCount; ++x)
         stream->write(mColor[x]);
      stream->write(mOffsetSpeed);
      stream->write(mMinVelocity);
      stream->write(mMaxVelocity);
      stream->write(mMaxDrops);
      stream->write(mRadius);

      if(stream->writeFlag((mStormData.currentTime / 32.0f) < mStormData.time))
      {
         stream->write(mStormData.currentTime);
         stream->write(mStormData.endPercentage);  
         stream->write(mStormData.time);  
      }
   }

   if(stream->writeFlag(mask & StormShowMask))
      stream->write(mStormPrecipitationOn);

   if(stream->writeFlag(mask & StormMask))
   {
      stream->write(mStormData.endPercentage);  
      stream->write(mStormData.time);  
      mStormData.currentTime = 0.0f;
   }

   if(stream->writeFlag(mask & PercentageMask))
      stream->write(mPercentage);

   return 0;
}

void Precipitation::unpackUpdate(NetConnection* con, BitStream* stream)
{
   Parent::unpackUpdate(con, stream);

   if(stream->readFlag())
   {
      stream->read(&mPercentage);
      stream->read(&mColorCount);
      for(S32 x = 0; x < mColorCount; ++x)
         stream->read(&mColor[x]);
      stream->read(&mOffsetSpeed);
      stream->read(&mMinVelocity);
      stream->read(&mMaxVelocity);
      stream->read(&mMaxDrops);
      stream->read(&mRadius);
      
      if(stream->readFlag())
      {
         stream->read(&mStormData.currentTime);
         stream->read(&mStormData.endPercentage);
         stream->read(&mStormData.time);
   
         F32 percentage = mStormData.endPercentage;

         mStormData.speed =  (percentage - mPercentage) / (mStormData.time * 32.0f); 
         mStormData.endPercentage = percentage;      
         mStormData.state = (mPercentage > percentage) ? out : in;
         mPercentage += (mStormData.state == in) ? mStormData.speed * mStormData.currentTime : 
                                                   -mStormData.speed * mStormData.currentTime;
         mStormPrecipitationOn = true;

      }
   }
   if(stream->readFlag())
   {
      stream->read(&mStormPrecipitationOn);
      if(!mStormPrecipitationOn)
         mPercentage = 0.0f;
   }      
      
   if(stream->readFlag())
   {
      stream->read(&mStormData.endPercentage);
      stream->read(&mStormData.time);
         
      if(mStormData.time)
         startStorm();
   }

   if(stream->readFlag())
   {
      stream->read(&mPercentage);
      mNumDrops = (S32)(mMaxDrops * mPercentage);
   }
}

void Precipitation::onRemove()
{
   removeFromScene();

   Parent::onRemove();
}

bool Precipitation::onNewDataBlock(GameBaseData* dptr)
{
   mDataBlock = dynamic_cast<PrecipitationData*>(dptr);
   if (!mDataBlock || !Parent::onNewDataBlock(dptr))
      return false;
   
   setDefaultValues();

   mAverageSpeed = ((mMinVelocity + mMaxVelocity) / 2.0f) * 0.65f;
   scriptOnNewDataBlock();
   return true;
}

void Precipitation::loadDml()
{
   S32 x;
   mNumTextures = 0;      
   Stream *stream = ResourceManager->openStream(mDataBlock->mMaterialListName);
   if(stream)
   {
      // match this code to the new sky loading code
      char path[1024], *p;
      dStrcpy(path, mDataBlock->mMaterialListName);
      if ((p = dStrrchr(path, '/')) != NULL)
         *p = 0;

      mMaterialList.read(*stream);
      ResourceManager->closeStream(stream);
      mMaterialList.load(path);
      for(x = 0; x < mMaterialList.size(); ++x, ++mNumTextures)
         mTextures[x] = mMaterialList.getMaterial(x); 
   }
}

//--------------------------------------------------------------------------
bool Precipitation::prepRenderImage(SceneState* state, const U32 stateKey,
                                    const U32 /*startZone*/, const bool /*modifyBaseState*/)
{
   if (!smPrecipitationOn)
   {
      if (mAudioHandle)
      {
         alxStop(mAudioHandle);
         mAudioHandle = 0;
      }
      return false;
   }
   else
   {
      if(!mStormPrecipitationOn) 
         return false;
      if (!mAudioHandle && mDataBlock->soundProfile)
         mAudioHandle = alxPlay(mDataBlock->soundProfile, &getTransform());
   }

   if (isLastState(state, stateKey))
      return false;
   setLastState(state, stateKey);

   // This should be sufficient for most objects that don't manage zones, and
   //  don't need to return a specialized RenderImage...
   if (state->isObjectRendered(this)) {
      SceneRenderImage* image = new SceneRenderImage;
      image->obj = this;
      image->isTranslucent = true;
      image->sortType = SceneRenderImage::EndSort;
      state->insertRenderImage(image);
   }

   return false;
}

void Precipitation::renderObject(SceneState* state, SceneRenderImage*)
{
   if(smPrecipitationOn)
   {
      if(!mStormPrecipitationOn) 
         return;
   }
   else
      return;

   AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on entry");
                           
   state->mModelview.getRow(0,&mRotX);
   state->mModelview.getRow(2,&mRotZ);

   RectI viewport;
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   dglGetViewport(&viewport);

   // Uncomment this if this is a "simple" (non-zone managing) object
   state->setupObjectProjection(this);

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();

   if(mStormData.state != done)
      updateStorm();
   if(mDataBlock->mType == 0 || mDataBlock->mType == 1)
      renderPrecip(state->getCameraPosition());
   else if(mDataBlock->mType == 2)
      renderSand();
 
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();

   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);
   dglSetViewport(viewport);

   mFirstTime = false;

   AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on exit");
}


//--------------------------------------------------------------------------
void Precipitation::advanceTime(F32 dt)
{
   if(!mFirstTime)
      mCurrentTime += dt;
}

void Precipitation::renderPrecip(Point3F camPos)
{
   Point3F point[4];
   F32 coordX, coordY;
   F32 xRadius, yRadius;
   mRotX *= mDataBlock->mSizeX;
   mRotZ *= mDataBlock->mSizeY;
   Point2F value;
   F32 renderPer = 1.0f, minSpeed = 0.0f;
   F32 theRadius, curZVelocity = 0.0f;
   F32 height, speed, radVal;
   F32 random1, random2, random3;
   mOffsetVal.set(0.0f, 0.0f);

   glEnable(GL_TEXTURE_2D);
   glEnable(GL_BLEND);
   glEnable(GL_ALPHA_TEST);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glAlphaFunc(GL_GREATER, 0.1f);
   glBindTexture(GL_TEXTURE_2D,mTextures[0].getGLName());
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

   calcVelocityBox(camPos, curZVelocity, renderPer);
   if(curZVelocity < 0.0f)
   {
      if(mDataBlock->mType == 0)
         minSpeed = ((mCurrentTime - mLastRenderTime) / (1.0f / curZVelocity)) - mMinVelocity;
   }
   glBegin(GL_QUADS);
   {
     for(S32 x=0; x < mNumDrops; ++x)
      {
         if(mStormData.numDrops > x || mFallingObj[x].stillFalling)
         {
            speed = mFallingObj[x].speed * (mCurrentTime - mLastRenderTime) * 32;
            if(smPrecipitationPause)
               speed = 0;
            else if(minSpeed < 0.0f)
                  speed = (mFallingObj[x].speed > minSpeed) ? minSpeed : mFallingObj[x].speed;
            
            mFallingObj[x].curPos.set((mFallingObj[x].curPos.x + mShiftPrecip.x) + (mFallingObj[x].offset.x * speed),           
                                      (mFallingObj[x].curPos.y + mShiftPrecip.y) + (mFallingObj[x].offset.y * speed),           
                                      (mFallingObj[x].curPos.z + mShiftPrecip.z) + speed);
                                                 
            if(mFallingObj[x].notValid || !mBox.isContained(mFallingObj[x].curPos))
            {
               if(mStormData.numDrops > x)
               {
                  xRadius = mRadius * ((mOffsetVal.x > 0.0f) ? 1.0f - mOffsetVal.x : 1.0f + mOffsetVal.x);
                  yRadius = mRadius * ((mOffsetVal.y > 0.0f) ? 1.0f - mOffsetVal.y : 1.0f + mOffsetVal.y);
                  
                  theRadius = (xRadius > yRadius) ? yRadius : xRadius;
                  
                  random1 = Platform::getRandom();
                  random2 = Platform::getRandom();
                  random3 = Platform::getRandom();

                  value.set( Platform::getRandom() * 2.0f - 1.0f, Platform::getRandom() * 2.0f - 1.0f);
                  value.normalize();
                  
                  mFallingObj[x].speed = -mMinVelocity + (-(mMaxVelocity - mMinVelocity) * random3);
                  if(random1 > (1.0f - renderPer) || (Platform::getRandom() < mDataBlock->tTopBoxDrawPer && renderPer < - 0.4))
                  {
                  //Start Drop on top of box
                     if(renderPer < -0.4)
                        mFallingObj[x].speed *= mDataBlock->tFrontSpeedPer * 2.5f;
                     if(mRandomHeight)
                        height = (mBox.max.z - 5.0f)  * (Platform::getRandom() * 2.0f - 1.0f);// + Platform::getRandom() * 30;   
                     else
                        height = mBox.max.z - 5.0f;
                     mFallingObj[x].dropType = 1;
                      radVal = theRadius * mClampF(((random2 < 0.2f) ? random3 : random2), 
                                                    0.01f, 1.0f);
                  }
                  else if(random1 > (0.0f - renderPer))
                  {
                  //Start Drop on side of box
                     height = random3 * mBox.max.z;
                     radVal = theRadius * mDataBlock->tFrontRadiusPer;
                     mFallingObj[x].speed *= mDataBlock->tFrontSpeedPer;
                     mFallingObj[x].dropType = 2;
                  }
                  else
                  {
                  //Start Drop on bottom of box
                     height = mBox.min.z + mDataBlock->tBottomDrawHeight;
                     mFallingObj[x].dropType = 3;
                     radVal = theRadius * random3;
                     mFallingObj[x].speed *= mDataBlock->tBottomSpeedPer;
                  }
                  value *= radVal;
                  mFallingObj[x].colorIndex = (S32)(mCeil(mColorCount * random2) - 1.0f);
                  mFallingObj[x].startPos.set(camPos.x + mNewCenter.x + value.x, camPos.y + mNewCenter.y + value.y, height);
                  mFallingObj[x].curPos = mFallingObj[x].startPos;
                  mFallingObj[x].startDiff = mFallingObj[x].startPos.z - camPos.z + 20;
                  mFallingObj[x].offset = mOffset; 
                  mFallingObj[x].texIndex = (S32)(mCeil((NUM_TEXTURES - 1) * getRandomVal()) - 1.0f);
                  mFallingObj[x].stillFalling = true;
                  mFallingObj[x].notValid = false;
               }
               else
               {
                  mFallingObj[x].stillFalling = false;
               }
            }
      
            if(mFallingObj[x].stillFalling)// && mBox.isContained(mFallingObj[x].curPos))
            {
               coordX = mTexCoord[mFallingObj[x].texIndex].x;
               coordY = mTexCoord[mFallingObj[x].texIndex].y;

               point[1] = mFallingObj[x].curPos + mRotX - mRotZ;   
               point[2] = mFallingObj[x].curPos - mRotX - mRotZ;   
               if( mDataBlock->mType == 1 )
               {
                  point[0] = mFallingObj[x].curPos + mRotX + mRotZ;   
                  point[3] = mFallingObj[x].curPos - mRotX + mRotZ;   
               }
               else
               {
                  point[0].set(point[1].x + (mFallingObj[x].offset.x * mDataBlock->mSizeY * 2), point[1].y + (mFallingObj[x].offset.y * mDataBlock->mSizeY * 2), point[1].z + (mFallingObj[x].offset.z * mDataBlock->mSizeY * 2));        
                  point[3].set(point[2].x + (mFallingObj[x].offset.x * mDataBlock->mSizeY * 2), point[2].y + (mFallingObj[x].offset.y * mDataBlock->mSizeY * 2), point[2].z + (mFallingObj[x].offset.z * mDataBlock->mSizeY * 2)); 
               }
               
               if(mFallingObj[x].dropType == 1)
               {
                  F32 alphaVal = 1.0f - ((mFallingObj[x].curPos.z - camPos.z) / mFallingObj[x].startDiff);
                  glColor4f(mColor[mFallingObj[x].colorIndex].red,
                            mColor[mFallingObj[x].colorIndex].green,
                            mColor[mFallingObj[x].colorIndex].blue,
                            alphaVal);
               }
               else
                  glColor4f(mColor[mFallingObj[x].colorIndex].red,
                            mColor[mFallingObj[x].colorIndex].green,
                            mColor[mFallingObj[x].colorIndex].blue,
                            1.0f);

               glTexCoord2f(coordX, coordY);
               glVertex3f(point[0].x, point[0].y, point[0].z);
               glTexCoord2f(coordX, coordY + 0.25);
               glVertex3f(point[1].x, point[1].y, point[1].z);
               glTexCoord2f(coordX + 0.25, coordY + 0.25);
               glVertex3f(point[2].x, point[2].y, point[2].z);
               glTexCoord2f(coordX + 0.25, coordY);
               glVertex3f(point[3].x, point[3].y, point[3].z);
            }
         }
      }
   }
   glEnd();
   glDisable(GL_TEXTURE_2D);
   glDisable(GL_ALPHA_TEST);
   glDisable(GL_BLEND);
   mLastRenderTime = mCurrentTime;
   mRandomHeight = false;
}

void Precipitation::renderSand()
{
   Point3F objPos;
   F32 dt, modVal;

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   for(S32 x=0; x < mNumDrops; ++x)
   {
      if(mStormData.numDrops > x || mFallingObj[x].stillFalling)
      {
         mFallingObj[x].offset.x += mOffset.x;
         mFallingObj[x].offset.y += mOffset.y;
         if(mFallingObj[x].endTime < mCurrentTime)
         {
            if(mStormData.numDrops > x)
            {
               modVal = mFmod(mCurrentTime - mFallingObj[x].endTime, mFallingObj[x].endTime - mFallingObj[x].startTime);
               mFallingObj[x].startTime = mCurrentTime - modVal;
               mFallingObj[x].endTime = (mCurrentTime + ((mMinVelocity * Platform::getRandom())+mMaxVelocity)) - modVal;
               mFallingObj[x].size = (mDataBlock->mMaxSize * Platform::getRandom());
               mFallingObj[x].offset.set(0.0, 0.0, 0.0);
               mFallingObj[x].colorIndex = (S32)(mCeil(mColorCount * Platform::getRandom()) - 1.0f);
               mFallingObj[x].stillFalling = true;
            }
            else
               mFallingObj[x].stillFalling = false;
         }
         dt = (mFallingObj[x].endTime - mCurrentTime) / 
            (mFallingObj[x].endTime - mFallingObj[x].startTime); 

         glPointSize(mFallingObj[x].size);
         glColor4f(mColor[mFallingObj[x].colorIndex].red, mColor[mFallingObj[x].colorIndex].green, mColor[mFallingObj[x].colorIndex].blue, 1.0f - dt);
         objPos = Point3F((mFallingObj[x].startPos.x * mRadius)+mFallingObj[x].offset.x,           
                          (mFallingObj[x].startPos.y * mRadius)+mFallingObj[x].offset.y,           
                          (mFallingObj[x].startPos.z * (dt * mRadius)) - mRadius);
         glBegin(GL_POINTS);
         glVertex3f(objPos.x, objPos.y, objPos.z);   
         glEnd();   
      }
   }
   glDisable(GL_BLEND);
}

void Precipitation::calcVelocityBox(Point3F camPos, F32 &curZVelocity, F32 &renderPer)
{
   GameConnection * con = GameConnection::getServerConnection();
   if(!con)
      return;

   Sky* pSky = mSceneManager->getCurrentSky();
   mOffset.set(0.0f, 0.0f, 1.0f);
   if (pSky)
      if(pSky->mEffectPrecip)
         mOffset.set(pSky->mWindDir.x * mOffsetSpeed, pSky->mWindDir.y * mOffsetSpeed, 1.0f);
   
   F32 radZ, speed;
   F32 randomVal, bottomHeight;
   Player *player = NULL;
   
   mNewCenter.set(mOffset.x * mRadius, mOffset.y * mRadius, 0.0f);
   mShiftPrecip.set(0.0f, 0.0f, 0.0f);
   if((camPos - mLastPos).len() > mRadius / 2.0f)
      mShiftPrecip = camPos - mLastPos;
   
   mLastPos = camPos;                      
   bottomHeight = (camPos.z - (mRadius / mDataBlock->tDivHeightVal));
   
   ShapeBase * conObj = con->getControlObject();
   if(conObj->getWaterCoverage()) 
      bottomHeight = conObj->getLiquidHeight() + mDataBlock->mSizeY;
   
   player = dynamic_cast<Player*>(conObj);
   if(!player)
   {
      mBox.min.set(camPos.x - mRadius, camPos.y - mRadius, bottomHeight);
      mBox.max.set(camPos.x + mRadius, camPos.y + mRadius, camPos.z + (mRadius / mDataBlock->tDivHeightVal));
      return;
   }
   Point3F curVel(player->getVelocity());
   F32 percentage = 0.0f;   
   F32 movePer = mDataBlock->tMoveingBoxPer;

   //Calc the offset of the inner box along x and y
   if(curVel.x)
   {
      percentage = curVel.x / 40.0f;
      mOffsetVal.x = (curVel.x > 0.0f) ?
              (percentage > movePer) ? movePer : percentage :
              (percentage < -movePer) ? -movePer : percentage;
   }
   if(curVel.y)
   {
      percentage = curVel.y / 40.0f;
      mOffsetVal.y = (curVel.y > 0.0f) ?
              (percentage > movePer) ? movePer : percentage :
              (percentage < -movePer) ? -movePer : percentage;
   }

   curZVelocity = curVel.z;
   speed = Point2F(curVel.x, curVel.y).len();
   radZ = mRadius;
   if(speed > 1.0f)
   {
      F32 val = 40.0f / speed;
      radZ *= (val > 1.0f) ? 1.0f : (val < MIN_ZRADIUS) ? MIN_ZRADIUS : val;
   }   
   //Move the box up slow... Helps keep snow around the player...
   F32 topHeight = (radZ / mDataBlock->tDivHeightVal);
   if(topHeight > mLastHeight && (topHeight - mLastHeight) > 5.0f)
      topHeight += 5.0f;
   mLastHeight = topHeight;   

   if(mDataBlock->tSizeBigBox)
   {
      mBox.min.set(camPos.x - (mRadius - (mRadius * mOffsetVal.x)), 
                   camPos.y - (mRadius - (mRadius * mOffsetVal.y)), bottomHeight);
      mBox.max.set(camPos.x + (mRadius + (mRadius * mOffsetVal.x)), 
                   camPos.y + (mRadius + (mRadius * mOffsetVal.y)), camPos.z + topHeight);
   }
   else
   {
      mBox.min.set(camPos.x - mRadius, camPos.y - mRadius, camPos.z - (mRadius / mDataBlock->tDivHeightVal));
      mBox.max.set(camPos.x + mRadius, camPos.y + mRadius, camPos.z + (mRadius / mDataBlock->tDivHeightVal));
   }                     
  
   speed = Point3F(curVel.x, curVel.y, curVel.z).len();
   randomVal = Platform::getRandom();
   if(speed < mDataBlock->tTopBoxSpeed)
      renderPer = 1.0f;
   else if ( speed < mDataBlock->tFrontBoxSpeed )
   {   
      if(randomVal < 0.25)
         renderPer = 1.0f;
      else
         renderPer = (curVel.z / speed) + (1.0f - ((speed - mDataBlock->tTopBoxSpeed)/ (mDataBlock->tFrontBoxSpeed - mDataBlock->tTopBoxSpeed)));   
   }
   else
   {
      if(randomVal < 0.25)
         renderPer = 1.0f;
      else 
         renderPer = curVel.z / speed;
   }

   if(speed)
      curVel.normalize();
   else
      curVel.set(0.0f, 0.0f, 0.0f);

   mNewCenter.set(curVel.x * mAbs((S32)(mOffsetVal.x * mRadius)), curVel.y * mAbs((S32)(mOffsetVal.y * mRadius)), curVel.z * mRadius);

   if(mAverageSpeed && mOffset.z && (mOffset.x || mOffset.y))
      mNewCenter.set(mNewCenter.x + ((mRadius / (mAverageSpeed * mOffset.z)) * mOffset.x),
                    mNewCenter.y + ((mRadius / (mAverageSpeed * mOffset.z)) * mOffset.y),
                    0.0f);
}

void Precipitation::setPercentage(F32 newPer)
{
   if(newPer <= 1.0f && newPer >= 0.0f)
   {
      mPercentage = newPer;
      mNumDrops = (S32)(mMaxDrops * mPercentage);
      setMaskBits(PercentageMask);
   }   
}

void Precipitation::stormShow(bool show)
{
   mStormPrecipitationOn = show;
   setMaskBits(StormShowMask);
}

void Precipitation::setupStorm(F32 percentage, F32 time)
{
   mStormData.time = time;
   if(mStormData.endPercentage >= 0.0f)
   {
      mStormData.state = (mStormData.endPercentage > percentage) ? out : in;
      mPercentage = mStormData.endPercentage;
   }
   else
      mStormData.state = (mPercentage > percentage) ? out : in;
   mStormData.endPercentage = percentage;
   
   setMaskBits(StormMask);
}

void Precipitation::startStorm()
{
   F32 percentage = mStormData.endPercentage;

   mStormData.speed =  (percentage - mPercentage) / (mStormData.time * 32.0f); 
   mStormData.endPercentage = percentage;      
   mStormData.state = (mPercentage > percentage) ? out : in;
   mStormPrecipitationOn = true;
}

void Precipitation::updateStorm()
{
   F32 offset = 1.0f;
   U32 currentTime = Sim::getCurrentTime();
   if(mStormData.lastTime != 0)
      offset = (currentTime - mStormData.lastTime) / 32.0f;

   mStormData.lastTime = currentTime;

   mPercentage += (mStormData.speed * offset);
   if((mStormData.state == in && mPercentage >= mStormData.endPercentage) || 
      (mStormData.state == out && mPercentage <= mStormData.endPercentage))
   {
      mPercentage = mStormData.endPercentage;
      mStormData.state = done;
      mStormData.lastTime = 0.0f;
   }
   mStormData.numDrops = (S32)(mMaxDrops * mPercentage);
}

void Precipitation::processTick(const Move* move)
{
   Parent::processTick(move);
   mStormData.currentTime++;   
}

void Precipitation::setupTexCoords()
{
   S32 x, y, numTimes = NUM_TEXTURES / 4;
   
   for(y = 0; y < numTimes ; ++y)  
      for(x = 0; x < numTimes ; ++x)  
         mTexCoord[y * numTimes + x].set(x * 0.25f, y * 0.25f);
}

F32 Precipitation::getRandomVal()
{
   F32 randVal = Platform::getRandom();
   if(randVal > 0.99995f && randVal < 0.99999f)
      randVal = 1.01f;
   return randVal;
}
