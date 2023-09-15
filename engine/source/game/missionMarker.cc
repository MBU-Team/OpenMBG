//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "game/missionMarker.h"
#include "console/consoleTypes.h"
#include "dgl/dgl.h"
#include "core/color.h"

extern bool gEditingMission;
IMPLEMENT_CO_DATABLOCK_V1(MissionMarkerData);

//------------------------------------------------------------------------------
// Class: MissionMarker
//------------------------------------------------------------------------------
IMPLEMENT_CO_NETOBJECT_V1(MissionMarker);

MissionMarker::MissionMarker()
{
   mTypeMask |= StaticShapeObjectType | StaticObjectType;
   mDataBlock = 0;
   mAddedToScene = false;
   mNetFlags.set(Ghostable | ScopeAlways);
}

bool MissionMarker::onAdd()
{
   if(!Parent::onAdd() || !mDataBlock)
      return(false);

   if(gEditingMission)
   {
      addToScene();
      mAddedToScene = true;
   }
   return(true);
}

void MissionMarker::onRemove()
{
   if(gEditingMission)
   {
      removeFromScene();
      mAddedToScene = false;
   }

   Parent::onRemove();
}

void MissionMarker::inspectPostApply()
{
   Parent::inspectPostApply();
   setMaskBits(PositionMask);
}

void MissionMarker::onEditorEnable()
{
   if(!mAddedToScene)
   {
      addToScene();
      mAddedToScene = true;
   }
}

void MissionMarker::onEditorDisable()
{
   if(mAddedToScene)
   {
      removeFromScene();
      mAddedToScene = false;
   }
}

bool MissionMarker::onNewDataBlock(GameBaseData * dptr)
{
   mDataBlock = dynamic_cast<MissionMarkerData*>(dptr);
   if(!mDataBlock || !Parent::onNewDataBlock(dptr))
      return(false);
   scriptOnNewDataBlock();
   return(true);
}

void MissionMarker::setTransform(const MatrixF& mat)
{
   Parent::setTransform(mat);
   setMaskBits(PositionMask);
}

U32 MissionMarker::packUpdate(NetConnection * con, U32 mask, BitStream * stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);
   if(stream->writeFlag(mask & PositionMask))
   {
      stream->writeAffineTransform(mObjToWorld);
      mathWrite(*stream, mObjScale);
   }

   return(retMask);
}

void MissionMarker::unpackUpdate(NetConnection * con, BitStream * stream)
{
   Parent::unpackUpdate(con, stream);
   if(stream->readFlag())
   {
      MatrixF mat;
      stream->readAffineTransform(&mat);
      Parent::setTransform(mat);
                                          
      Point3F scale;
      mathRead(*stream, &scale);
      setScale(scale);
   }
}

void MissionMarker::initPersistFields() {
   Parent::initPersistFields();
}

//------------------------------------------------------------------------------
// Class: WayPoint
//------------------------------------------------------------------------------
IMPLEMENT_CO_NETOBJECT_V1(WayPoint);

WayPointTeam::WayPointTeam()
{
   mTeamId = 0;
   mWayPoint = 0;
}

WayPoint::WayPoint()
{
   mName = StringTable->insert("");
}

void WayPoint::setHidden(bool hidden)
{
   if(isServerObject())
      setMaskBits(UpdateHiddenMask);
   mHidden = hidden;
}

bool WayPoint::onAdd()
{
   if(!Parent::onAdd())
      return(false);

   //
   if(isClientObject())
      Sim::getWayPointSet()->addObject(this);
   else
   {
      mTeam.mWayPoint = this;
      setMaskBits(UpdateNameMask|UpdateTeamMask);
   }

   return(true);
}

void WayPoint::inspectPostApply()
{
   Parent::inspectPostApply();
   if(!mName || !mName[0])
      mName = StringTable->insert("");
   setMaskBits(UpdateNameMask|UpdateTeamMask);
}

U32 WayPoint::packUpdate(NetConnection * con, U32 mask, BitStream * stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);
   if(stream->writeFlag(mask & UpdateNameMask))
      stream->writeString(mName);
   if(stream->writeFlag(mask & UpdateTeamMask))
      stream->write(mTeam.mTeamId);
   if(stream->writeFlag(mask & UpdateHiddenMask))
      stream->writeFlag(mHidden);
   return(retMask);
}
                   
void WayPoint::unpackUpdate(NetConnection * con, BitStream * stream)
{
   Parent::unpackUpdate(con, stream);
   if(stream->readFlag())
      mName = stream->readSTString(true);
   if(stream->readFlag())
      stream->read(&mTeam.mTeamId);
   if(stream->readFlag())
      mHidden = stream->readFlag();
}

static const char * getDataTypeWayPointTeam(void * dptr, EnumTable *, BitSet32)
{
   char * buf = Con::getReturnBuffer(32);
   dSprintf(buf, 32, "%d", ((WayPointTeam*)dptr)->mTeamId);
   return(buf);
}

static void setDataTypeWayPointTeam(void * dptr, S32, const char ** argv, EnumTable *, BitSet32)
{
   WayPointTeam * pTeam = (WayPointTeam*)dptr;
   pTeam->mTeamId = dAtoi(argv[0]);

   if(pTeam->mWayPoint && pTeam->mWayPoint->isServerObject())
      pTeam->mWayPoint->setMaskBits(WayPoint::UpdateTeamMask);
}

void WayPoint::initPersistFields()
{
   Parent::initPersistFields();

   Con::registerType("WaypointTeam", TypeWayPointTeam, sizeof(WayPointTeam), getDataTypeWayPointTeam, setDataTypeWayPointTeam);

   addGroup("Misc");	// MM: Added Group Header.
   addField("name", TypeCaseString, Offset(mName, WayPoint));
   addField("team", TypeWayPointTeam, Offset(mTeam, WayPoint));
   endGroup("Misc");	// MM: Added Group Footer.
}

//------------------------------------------------------------------------------
// Class: SpawnSphere
//------------------------------------------------------------------------------
IMPLEMENT_CO_NETOBJECT_V1(SpawnSphere);

Sphere SpawnSphere::smSphere(Sphere::Octahedron);

SpawnSphere::SpawnSphere()
{
   mRadius = 100.f;
   mSphereWeight = 100.f;
   mIndoorWeight = 100.f;
   mOutdoorWeight = 100.f;
}

bool SpawnSphere::onAdd()
{
   if(!Parent::onAdd())
      return(false);

   if(!isClientObject())
      setMaskBits(UpdateSphereMask);

   return true;
}

void SpawnSphere::inspectPostApply()
{
   Parent::inspectPostApply();
   setMaskBits(UpdateSphereMask);
}

U32 SpawnSphere::packUpdate(NetConnection * con, U32 mask, BitStream * stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);

   // 
   if(stream->writeFlag(mask & UpdateSphereMask))
   {
      stream->write(mRadius);
      stream->write(mSphereWeight);
      stream->write(mIndoorWeight);
      stream->write(mOutdoorWeight);
   }
   return(retMask);
}

void SpawnSphere::unpackUpdate(NetConnection * con, BitStream * stream)
{
   Parent::unpackUpdate(con, stream);
   if(stream->readFlag())
   {
      stream->read(&mRadius);
      stream->read(&mSphereWeight);
      stream->read(&mIndoorWeight);
      stream->read(&mOutdoorWeight);
   }
}

void SpawnSphere::initPersistFields()
{
   Parent::initPersistFields();

   addGroup("Dimensions");	// MM: Added Group Header.
   addField("radius", TypeF32, Offset(mRadius, SpawnSphere));
   endGroup("Dimensions");	// MM: Added Group Footer.

   addGroup("Weight");	// MM: Added Group Header.
   addField("sphereWeight", TypeF32, Offset(mSphereWeight, SpawnSphere));
   addField("indoorWeight", TypeF32, Offset(mIndoorWeight, SpawnSphere));
   addField("outdoorWeight", TypeF32, Offset(mOutdoorWeight, SpawnSphere));
   endGroup("Weight");	// MM: Added Group Footer.
}
