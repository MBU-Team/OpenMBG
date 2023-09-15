//-----------------------------------------------------------------------------
// Torque Game Engine 
// Written by Melvyn May, Started on 4th August 2002.
//
// "My code is written for the Torque community, so do your worst with it,
//	just don't rip-it-off and call it your own without even thanking me".
//
//	- Melv.
//
//-----------------------------------------------------------------------------

#include "dgl/dgl.h"
#include "console/consoleTypes.h"
#include "core/bitStream.h"
#include "math/mRandom.h"
#include "math/mathIO.h"
#include "terrain/terrData.h"
#include "game/gameConnection.h"
#include "console/simBase.h"
#include "sceneGraph/sceneGraph.h"
#include "fxFoliageReplicator.h"

//------------------------------------------------------------------------------
//
//	Put the function in /example/common/editor/ObjectBuilderGui.gui [around line 458] ...
//
//	function ObjectBuilderGui::buildfxFoliageReplicator(%this)
//	{
//		%this.className = "fxFoliageReplicator";
//		%this.process();
//	}
//
//------------------------------------------------------------------------------
//
//	Put this in /example/common/editor/EditorGui.cs in [function Creator::init( %this )]
//
//   %Environment_Item[8] = "fxFoliageReplicator";  <-- ADD THIS.
//
//------------------------------------------------------------------------------
//
//	Put this in /example/common/client/missionDownload.cs in [function clientCmdMissionStartPhase3(%seq,%missionName)] (line 65)
//	after codeline 'onPhase2Complete();'.
//
//	StartFoliageReplication();
//
//------------------------------------------------------------------------------
//
//	Put this in /engine/console/simBase.h (around line 509) in
//
//	namespace Sim
//  {
//	   DeclareNamedSet(fxFoliageSet)  <-- ADD THIS (Note no semi-colon).
//
//------------------------------------------------------------------------------
//
//	Put this in /engine/console/simBase.cc (around line 19) in
//
//  ImplementNamedSet(fxFoliageSet)  <-- ADD THIS (Note no semi-colon).
//
//------------------------------------------------------------------------------
//
//	Put this in /engine/console/simManager.cc [function void init()] (around line 269).
//
//	namespace Sim
//  {
//		InstantiateNamedSet(fxFoliageSet);  <-- ADD THIS (Including Semi-colon).
//
//------------------------------------------------------------------------------
extern bool gEditingMission;

//------------------------------------------------------------------------------

IMPLEMENT_CO_NETOBJECT_V1(fxFoliageReplicator);


//------------------------------------------------------------------------------
//
// Trig Table Lookups.
//
//------------------------------------------------------------------------------
static bool						mTrigTableInitialised;
static F32						mCosTable[720];
static F32						mSinTable[720];


//------------------------------------------------------------------------------
//
// Class: fxFoliageRenderList
//
//------------------------------------------------------------------------------

void fxFoliageRenderList::SetupClipPlanes(SceneState* state, const F32 FarClipPlane)
{
	// Fetch Camera Position.
	CameraPosition  = state->getCameraPosition();
	// Calculate Perspective.
	F32 FarOverNear = FarClipPlane / state->getNearPlane();

	// Calculate Clip-Planes.
	FarPosLeftUp    = Point3F(	state->getBaseZoneState().frustum[0] * FarOverNear,
								FarClipPlane,
								state->getBaseZoneState().frustum[3] * FarOverNear);
	FarPosLeftDown  = Point3F(	state->getBaseZoneState().frustum[0] * FarOverNear,
								FarClipPlane,
								state->getBaseZoneState().frustum[2] * FarOverNear);
	FarPosRightUp   = Point3F(	state->getBaseZoneState().frustum[1] * FarOverNear,
								FarClipPlane,
								state->getBaseZoneState().frustum[3] * FarOverNear);
	FarPosRightDown = Point3F(	state->getBaseZoneState().frustum[1] * FarOverNear,
								FarClipPlane,
								state->getBaseZoneState().frustum[2] * FarOverNear);

	// Calculate our World->Object Space Transform.
	MatrixF InvXForm = state->mModelview;
	InvXForm.inverse();
	// Convert to Object-Space.
	InvXForm.mulP(FarPosLeftUp);
	InvXForm.mulP(FarPosLeftDown);
	InvXForm.mulP(FarPosRightUp);
	InvXForm.mulP(FarPosRightDown);

	// Calculate Bounding Box (including Camera).
	mBox.min = CameraPosition;
	mBox.min.setMin(FarPosLeftUp);
	mBox.min.setMin(FarPosLeftDown);
	mBox.min.setMin(FarPosRightUp);
	mBox.min.setMin(FarPosRightDown);
	mBox.max = CameraPosition;
	mBox.max.setMax(FarPosLeftUp);
	mBox.max.setMax(FarPosLeftDown);
	mBox.max.setMax(FarPosRightUp);
	mBox.max.setMax(FarPosRightDown);

	// Setup Our Viewplane.
	ViewPlanes[0].set(CameraPosition,	FarPosLeftUp,		FarPosLeftDown);
	ViewPlanes[1].set(CameraPosition,	FarPosRightUp,		FarPosLeftUp);
	ViewPlanes[2].set(CameraPosition,	FarPosRightDown,	FarPosRightUp);
	ViewPlanes[3].set(CameraPosition,	FarPosLeftDown,		FarPosRightDown);
	ViewPlanes[4].set(FarPosLeftUp,		FarPosRightUp,		FarPosRightDown);
}

//------------------------------------------------------------------------------


inline void fxFoliageRenderList::DrawQuadBox(const Box3F& QuadBox, const ColorF Colour)
{
	// Define our debug box.
	static Point3F BoxPnts[] = {
								  Point3F(0,0,0),
								  Point3F(0,0,1),
								  Point3F(0,1,0),
								  Point3F(0,1,1),
								  Point3F(1,0,0),
								  Point3F(1,0,1),
								  Point3F(1,1,0),
								  Point3F(1,1,1)
								};

	static U32 BoxVerts[][4] = {
								  {0,2,3,1},     // -x
								  {7,6,4,5},     // +x
								  {0,1,5,4},     // -y
								  {3,2,6,7},     // +y
								  {0,4,6,2},     // -z
								  {3,7,5,1}      // +z   
								};

	static Point3F BoxNormals[] = {
								  Point3F(-1, 0, 0),
								  Point3F( 1, 0, 0),
								  Point3F( 0,-1, 0),
								  Point3F( 0, 1, 0),
								  Point3F( 0, 0,-1),
								  Point3F( 0, 0, 1)
								};

	// Select the Box Colour.
	glColor4fv(Colour);

	// Project our Box Points.
	Point3F ProjectionPoints[8];
	for(U32 i = 0; i < 8; i++)
	{  
		ProjectionPoints[i].set(BoxPnts[i].x ? QuadBox.max.x : QuadBox.min.x, 
								BoxPnts[i].y ? QuadBox.max.y : QuadBox.min.y,
								BoxPnts[i].z ? (mHeightLerp * QuadBox.max.z) + (1-mHeightLerp) * QuadBox.min.z : QuadBox.min.z);

	}

	// Draw the Box.
	for(U32 x = 0; x < 6; x++)
	{
		// Draw a line-loop.
		glBegin(GL_LINE_LOOP);

		for(U32 y = 0; y < 4; y++)
		{
			// Draw Vertex.
			glVertex3f(	ProjectionPoints[BoxVerts[x][y]].x,
						ProjectionPoints[BoxVerts[x][y]].y,
						ProjectionPoints[BoxVerts[x][y]].z);
		}

		glEnd();
	}
}

//------------------------------------------------------------------------------

void fxFoliageRenderList::CompileVisibleSet(const fxFoliageQuadrantNode* pNode, const MatrixF& RenderTransform, const bool UseDebug)
{
	// Attempt to trivially reject the Node.
	//
	// Is any of the quadrant visible?
	if (IsQuadrantVisible(pNode->QuadrantBox, RenderTransform))
	{
		// Draw the Quad Box (Debug Only).
		if (UseDebug) DrawQuadBox(pNode->QuadrantBox, ColorF(0,.8,.1,.2));

		// Yes, so are we at sub-level 0?
		if (pNode->Level == 0)
		{
			// Yes, so merge visible object set with node render list.
			mVisObjectSet.merge(pNode->RenderList);
		}
		else
		{
			// No, so compile quadrants.
			for (U32 q = 0; q < 4; q++)
				if (pNode->QuadrantChildNode[q]) CompileVisibleSet(pNode->QuadrantChildNode[q], RenderTransform, UseDebug);
		}
	}
	else
	{
		// Draw the Quad Box (Debug Only).
		if (UseDebug) DrawQuadBox(pNode->QuadrantBox, ColorF(0,.1,8,.2));
	}

	return;
}

//------------------------------------------------------------------------------

bool fxFoliageRenderList::IsQuadrantVisible(const Box3F VisBox, const MatrixF& RenderTransform)
{
	// Can we trivially accept the visible box?
	if (mBox.isOverlapped(VisBox))
	{
		// Yes, so calculate Object-Space Box.
		MatrixF InvXForm = RenderTransform;
		InvXForm.inverse();
		Box3F OSBox = VisBox;
		InvXForm.mulP(OSBox.min);
		InvXForm.mulP(OSBox.max);

		// Yes, so fetch Box Center.
		Point3F Center;
		OSBox.getCenter(&Center);

		// Scale.
		Point3F XRad(OSBox.len_x() * 0.5, 0, 0);
		Point3F YRad(0, OSBox.len_y() * 0.5, 0);
		Point3F ZRad(0, 0, OSBox.len_z() * 0.5);

		// Render Transformation.
		RenderTransform.mulP(Center);
		RenderTransform.mulV(XRad);
		RenderTransform.mulV(YRad);
		RenderTransform.mulV(ZRad);

		// Check against View-planes.
		for (U32 i = 0; i < 5; i++)
		{
			// Reject if not visible.
			if (ViewPlanes[i].whichSideBox(Center, XRad, YRad, ZRad, Point3F(0, 0, 0)) == PlaneF::Back) return false;
		}

		// Visible.
		return true;
	}

	// Not visible.
	return false;
}



//------------------------------------------------------------------------------
//
// Class: fxFoliageCulledList
//
//------------------------------------------------------------------------------
fxFoliageCulledList::fxFoliageCulledList(Box3F SearchBox, fxFoliageCulledList* InVec)
{
	// Find the Candidates.
	FindCandidates(SearchBox, InVec);
}

//------------------------------------------------------------------------------

void fxFoliageCulledList::FindCandidates(Box3F SearchBox, fxFoliageCulledList* InVec)
{
	// Search the Culled List.
	for (U32 i = 0; i < InVec->GetListCount(); i++)
	{
		// Is this Box overlapping our search box?
		if (SearchBox.isOverlapped(InVec->GetElement(i)->FoliageBox))
		{
			// Yes, so add it to our culled list.
			mCulledObjectSet.push_back(InVec->GetElement(i));
		}
	}
}



//------------------------------------------------------------------------------
//
// Class: fxFoliageReplicator
//
//------------------------------------------------------------------------------

fxFoliageReplicator::fxFoliageReplicator()
{
	// Setup NetObject.
	mTypeMask |= StaticObjectType | StaticTSObjectType | StaticRenderedObjectType;
	mAddedToScene = false;
	mNetFlags.set(Ghostable | ScopeAlways);

	// Reset Client Replication Started.
	mClientReplicationStarted = false;

	// Reset Foliage Count.
	mCurrentFoliageCount = 0;

	// Reset Creation Area Angle Animation.
	mCreationAreaAngle = 0;

	// Reset Last Render Time.
	mLastRenderTime = 0;

	// Reset Foliage Nodes.
	mPotentialFoliageNodes = 0;
	// Reset Billboards Acquired.
	mBillboardsAcquired = 0;

	// Reset Frame Serial ID.
	mFrameSerialID = 0;
}

//------------------------------------------------------------------------------

fxFoliageReplicator::~fxFoliageReplicator()
{
}

//------------------------------------------------------------------------------

void fxFoliageReplicator::initPersistFields()
{
	// Initialise parents' persistent fields.
	Parent::initPersistFields();

	// Add out own persistent fields.
	addGroup( "Debugging" );	// MM: Added Group Header.
	addField( "UseDebugInfo",		TypeBool,		Offset( mFieldData.mUseDebugInfo,			fxFoliageReplicator ) );	
	addField( "DebugBoxHeight",		TypeF32,		Offset( mFieldData.mDebugBoxHeight,			fxFoliageReplicator ) );	
	addField( "HideFoliage",		TypeBool,		Offset( mFieldData.mHideFoliage,			fxFoliageReplicator ) );
	addField( "ShowPlacementArea",	TypeBool,		Offset( mFieldData.mShowPlacementArea,		fxFoliageReplicator ) );
	addField( "PlacementAreaHeight",TypeS32,		Offset( mFieldData.mPlacementBandHeight,	fxFoliageReplicator ) );
	addField( "PlacementColour",	TypeColorF,		Offset( mFieldData.mPlaceAreaColour,		fxFoliageReplicator ) );
	endGroup( "Debugging" );	// MM: Added Group Footer.

	addGroup( "Media" );	// MM: Added Group Header.
	addField( "Seed",				TypeS32,		Offset( mFieldData.mSeed,					fxFoliageReplicator ) );
	addField( "FoliageFile",		TypeFilename,	Offset( mFieldData.mFoliageFile,			fxFoliageReplicator ) );
	addField( "FoliageCount",		TypeS32,		Offset( mFieldData.mFoliageCount,			fxFoliageReplicator ) );
	addField( "FoliageRetries",		TypeS32,		Offset( mFieldData.mFoliageRetries,			fxFoliageReplicator ) );
	endGroup( "Media" );	// MM: Added Group Footer.

	addGroup( "Area" );	// MM: Added Group Header.
	addField( "InnerRadiusX",		TypeS32,		Offset( mFieldData.mInnerRadiusX,			fxFoliageReplicator ) );	
	addField( "InnerRadiusY",		TypeS32,		Offset( mFieldData.mInnerRadiusY,			fxFoliageReplicator ) );	
	addField( "OuterRadiusX",		TypeS32,		Offset( mFieldData.mOuterRadiusX,			fxFoliageReplicator ) );	
	addField( "OuterRadiusY",		TypeS32,		Offset( mFieldData.mOuterRadiusY,			fxFoliageReplicator ) );	
	endGroup( "Area" );	// MM: Added Group Footer.

	addGroup( "Dimensions" );	// MM: Added Group Header.
	addField( "MinWidth",			TypeF32,		Offset( mFieldData.mMinWidth,				fxFoliageReplicator ) );	
	addField( "MaxWidth",			TypeF32,		Offset( mFieldData.mMaxWidth,				fxFoliageReplicator ) );	
	addField( "MinHeight",			TypeF32,		Offset( mFieldData.mMinHeight,				fxFoliageReplicator ) );	
	addField( "MaxHeight",			TypeF32,		Offset( mFieldData.mMaxHeight,				fxFoliageReplicator ) );	
	addField( "FixAspectRatio",		TypeBool,		Offset( mFieldData.mFixAspectRatio,			fxFoliageReplicator ) );	
	addField( "FixSizeToMax",		TypeBool,		Offset( mFieldData.mFixSizeToMax,			fxFoliageReplicator ) );	
	addField( "OffsetZ",			TypeF32,		Offset( mFieldData.mOffsetZ,				fxFoliageReplicator ) );	
	addField( "RandomFlip",			TypeBool,		Offset( mFieldData.mRandomFlip,				fxFoliageReplicator ) );	
	endGroup( "Dimensions" );	// MM: Added Group Footer.

	addGroup( "Culling" );	// MM: Added Group Header.
	addField( "UseCulling",			TypeBool,		Offset( mFieldData.mUseCulling,				fxFoliageReplicator ) );	
	addField( "CullResolution",		TypeS32,		Offset( mFieldData.mCullResolution,			fxFoliageReplicator ) );	
	addField( "ViewDistance",		TypeF32,		Offset( mFieldData.mViewDistance,			fxFoliageReplicator ) );	
	addField( "ViewClosest",		TypeF32,		Offset( mFieldData.mViewClosest,			fxFoliageReplicator ) );	
	addField( "FadeInRegion",		TypeF32,		Offset( mFieldData.mFadeInRegion,			fxFoliageReplicator ) );	
	addField( "FadeOutRegion",		TypeF32,		Offset( mFieldData.mFadeOutRegion,			fxFoliageReplicator ) );	
	addField( "AlphaCutoff",		TypeF32,		Offset( mFieldData.mAlphaCutoff,			fxFoliageReplicator ) );	
	addField( "GroundAlpha",		TypeF32,		Offset( mFieldData.mGroundAlpha,			fxFoliageReplicator ) );	
	endGroup( "Culling" );	// MM: Added Group Footer.

	addGroup( "Animation" );	// MM: Added Group Header.
	addField( "SwayOn",				TypeBool,		Offset( mFieldData.mSwayOn,					fxFoliageReplicator ) );	
	addField( "SwaySync",			TypeBool,		Offset( mFieldData.mSwaySync,				fxFoliageReplicator ) );	
	addField( "SwayMagSide",		TypeF32,		Offset( mFieldData.mSwayMagnitudeSide,		fxFoliageReplicator ) );	
	addField( "SwayMagFront",		TypeF32,		Offset( mFieldData.mSwayMagnitudeFront,		fxFoliageReplicator ) );	
	addField( "MinSwayTime",		TypeF32,		Offset( mFieldData.mMinSwayTime,			fxFoliageReplicator ) );	
	addField( "MaxSwayTime",		TypeF32,		Offset( mFieldData.mMaxSwayTime,			fxFoliageReplicator ) );	
	endGroup( "Animation" );	// MM: Added Group Footer.

	addGroup( "Lighting" );	// MM: Added Group Header.
	addField( "LightOn",			TypeBool,		Offset( mFieldData.mLightOn,				fxFoliageReplicator ) );	
	addField( "LightSync",			TypeBool,		Offset( mFieldData.mLightSync,				fxFoliageReplicator ) );	
	addField( "MinLuminance",		TypeF32,		Offset( mFieldData.mMinLuminance,			fxFoliageReplicator ) );	
	addField( "MaxLuminance",		TypeF32,		Offset( mFieldData.mMaxLuminance,			fxFoliageReplicator ) );	
	addField( "LightTime",			TypeF32,		Offset( mFieldData.mLightTime,				fxFoliageReplicator ) );	
	endGroup( "Lighting" );	// MM: Added Group Footer.

	addGroup( "Restrictions" );	// MM: Added Group Header.
	addField( "AllowOnTerrain",		TypeBool,		Offset( mFieldData.mAllowOnTerrain,			fxFoliageReplicator ) );
	addField( "AllowOnInteriors",	TypeBool,		Offset( mFieldData.mAllowOnInteriors,		fxFoliageReplicator ) );
	addField( "AllowOnStatics",		TypeBool,		Offset( mFieldData.mAllowStatics,			fxFoliageReplicator ) );	
	addField( "AllowOnWater",		TypeBool,		Offset( mFieldData.mAllowOnWater,			fxFoliageReplicator ) );
	addField( "AllowWaterSurface",	TypeBool,		Offset( mFieldData.mAllowWaterSurface,		fxFoliageReplicator ) );
	addField( "AllowedTerrainSlope",TypeS32,		Offset( mFieldData.mAllowedTerrainSlope,	fxFoliageReplicator ) );
	endGroup( "Restrictions" );	// MM: Added Group Footer.
}

//------------------------------------------------------------------------------

void fxFoliageReplicator::CreateFoliage(void)
{
	F32				HypX, HypY;
	F32				Angle;
	U32				RelocationRetry;
	Point3F			FoliagePosition;
	Point3F			FoliageStart;
	Point3F			FoliageEnd;
	Point3F			FoliageScale;
	bool			CollisionResult;
	RayInfo			RayEvent;

	// Let's get a minimum bounding volume.
	Point3F	MinPoint( -0.5, -0.5, -0.5 );
	Point3F	MaxPoint(  0.5,  0.5,  0.5 );

	// Check Host.
	AssertFatal(isClientObject(), "Trying to create Foliage on Server, this is bad!")

	// Cannot continue without Foliage Texture!
	if (mFieldData.mFoliageFile == "") return;

	// Check that we can position somewhere!
	if (!(	mFieldData.mAllowOnTerrain ||
			mFieldData.mAllowOnInteriors ||
			mFieldData.mAllowStatics ||
			mFieldData.mAllowOnWater))
	{
		// Problem ...
		Con::warnf(ConsoleLogEntry::General, "fxFoliageReplicator - Could not place Foliage, All alloweds are off!");

		// Return here.
		return;
	}

	// Destroy Foliage if we've already got some.
	if (mCurrentFoliageCount != 0) DestroyFoliage();

	// Inform the user if culling has been disabled!
	if (!mFieldData.mUseCulling)
	{
		// Console Output.
		Con::printf("fxFoliageReplicator - Culling has been disabled!");
	}

	// ----------------------------------------------------------------------------------------------------------------------
	// > Calculate the Potential Foliage Nodes Required to achieve the selected culling resolution.
	// > Populate Quad-tree structure to depth determined by culling resolution.
	//
	// A little explanation is called for here ...
	//
	//			The approach to this problem has been choosen to make it *much* easier for
	//			the user to control the quad-tree culling resolution.  The user enters a single
	//			world-space value 'mCullResolution' which controls the highest resolution at
	//			which the replicator will check visibility culling.
	//
	//			example:	If 'mCullResolution' is 32 and the size of the replicated area is 128 radius
	//						(256 diameter) then this results in the replicator creating a quad-tree where
	//						there are 256/32 = 8x8 blocks.  Each of these can be checked to see if they
	//						reside within the viewing frustum and if not then they get culled therefore
	//						removing the need to parse all the billboards that occcupy that region.
	//						Most of the time you will get better than this as the culling algorithm will
	//						check the culling pyramid from the top to bottom e.g. the follow 'blocks'
	//						will be checked:-
	//
	//						 1 x 256 x 256 (All of replicated area)
	//						 4 x 128 x 128 (4 corners of above)
	//						16 x  64 x  64 (16 x 4 corners of above)
	//						etc.
	//
	//
	//	1.		First-up, the replicator needs to create a fixed-list of quad-tree nodes to work with.
	//
	//			To calculate this we take the largest outer-radius value set in the replicator and
	//			calculate how many quad-tree levels are required to achieve the selected 'mCullResolution'.
	//			One of the initial problems is that the replicator has seperate radii values for X & Y.
	//			This can lead to a culling resolution smaller in one axis than the other if there is a
	//			difference between the Outer-Radii.  Unfortunately, we just live with this as there is
	//			not much we can do here if we still want to allow the user to have this kind of
	//			elliptical placement control.
	//
	//			To calculate the number of nodes needed we using the following equation:-
	//
	//			Note:- We are changing the Logarithmic bases from 10 -> 2 ... grrrr!
	//
	//			Cr = mCullResolution
	//			Rs = Maximum Radii Diameter
	//
	//
	//				( Log10( Rs / Cr )       )
	//			int ( ---------------- + 0.5 )
	//				( Log10( 2 )             )
	//
	//					---------|
	//					 \
	//					  \			 n
	//					  /			4
	//					 /
	//					---------|
	//					   n = 0
	//
	//
	//			So basically we calculate the number of blocks in 1D at the highest resolution, then
	//			calculate the inverse exponential (base 2 - 1D) to achieve that quantity of blocks.
	//			We round that upto the next highest integer = e.  We then sum 4 to the power 0->e
	//			which gives us the correct number of nodes required.  e is also stored as the starting
	//			level value for populating the quad-tree (see 3. below).
	//
	//	2.		We then proceed to calculate the billboard positions as normal and calculate and assign
	//			each billboard a basic volume (rather than treat each as a point).  We need to take into
	//			account possible front/back swaying as well as the basic plane dimensions here.
	//			When all the billboards have been choosen we then proceed to populate the quad-tree.
	//
	//	3.		To populate the quad-tree we start with a box which completely encapsulates the volume
	//			occupied by all the billboards and enter into a recursive procedure to process that node.
	//			Processing this node involves splitting it into quadrants in X/Y untouched (for now).
	//			We then find candidate billboards with each of these quadrants searching using the
	//			current subset of shapes from the parent (this reduces the searching to a minimum and
	//			is very efficient).
	//
	//			If a quadrant does not enclose any billboards then the node is dropped otherwise it
	//			is processed again using the same procedure.
	//
	//			This happens until we have recursed through the maximum number of levels as calculated
	//			using the summation max (see equation above).  When level 0 is reached, the current list
	//			of enclosed objects is stored within the node (for the rendering algorithm).
	//
	//	4.		When this is complete we have finished here.  The next stage is when rendering takes place.
	//			An algorithm steps through the quad-tree from the top and does visibility culling on
	//			each box (with respect to the viewing frustum) and culls as appropriate.  If the box is
	//			visible then the next level is checked until we reach level 0 where the node contains
	//			a complete subset of billboards enclosed by the visible box.
	//
	//
	//	Using the above algorithm we can now generate *massive* quantities of billboards and (using the
	//	appropriate 'mCullResolution') only visible blocks of billboards will be processed.
	//
	//	- Melv.
	//			
	// ----------------------------------------------------------------------------------------------------------------------



	// ----------------------------------------------------------------------------------------------------------------------
	// Step 1.
	// ----------------------------------------------------------------------------------------------------------------------

	// Calculate the maximum dimension.
	F32 MaxDimension = 2 * ( (mFieldData.mOuterRadiusX > mFieldData.mOuterRadiusY) ? mFieldData.mOuterRadiusX : mFieldData.mOuterRadiusY );

	// Let's check that our cull resolution is not greater than half our maximum dimension (and less than 1).
	if (mFieldData.mCullResolution > (MaxDimension/2) || mFieldData.mCullResolution < 8)
	{
		// Problem ...
		Con::warnf(ConsoleLogEntry::General, "fxFoliageReplicator - Could create Foliage, invalid Culling Resolution!");
		Con::warnf(ConsoleLogEntry::General, "fxFoliageReplicator - Culling Resolution *must* be >=8 or <= %0.2f!", (MaxDimension/2));

		// Return here.
		return;
	}

	// Take first Timestamp.
	F32 mStartCreationTime = Platform::getRealMilliseconds();

	// Calculate the quad-tree levels needed for selected 'mCullResolution'.
	mQuadTreeLevels = (U32)(mCeil(mLog( MaxDimension / mFieldData.mCullResolution ) / mLog( 2.0f )));

	// Calculate the number of potential nodes required.
	mPotentialFoliageNodes = 0;
	for (U32 n = 0; n <= mQuadTreeLevels; n++)
		mPotentialFoliageNodes += (U32)(mCeil(mPow(4.0f, n)));	// Ceil to be safe!

	// ----------------------------------------------------------------------------------------------------------------------
	// Step 2.
	// ----------------------------------------------------------------------------------------------------------------------

	// Set Seed.
	RandomGen.setSeed(mFieldData.mSeed);

	// Have we setup the Trig Tables?
	if (!mTrigTableInitialised)
	{
		F32 tIdx = 0.0f;

		// No, so setup Tables.
		for (U32 idx = 0; idx < 720; idx++, tIdx+=M_2PI/720.0f)
		{
			mCosTable[idx] = mCos(tIdx);
			mSinTable[idx] = mSin(tIdx);
		}

		// Signal Trig Tables Initialised.
		mTrigTableInitialised = true;
	}

	// Add Foliage.
	for (U32 idx = 0; idx < mFieldData.mFoliageCount; idx++)
	{
		fxFoliageItem*	pFoliageItem;
		Point3F			FoliageOffsetPos;

		// Reset Relocation Retry.
		RelocationRetry = mFieldData.mFoliageRetries;

		// Find it a home ...
		do
		{
			// Get the fxFoliageReplicator Position.
			FoliagePosition = getPosition();

			// Calculate a random offset
			HypX	= RandomGen.randF(mFieldData.mInnerRadiusX, mFieldData.mOuterRadiusX);
			HypY	= RandomGen.randF(mFieldData.mInnerRadiusY, mFieldData.mOuterRadiusY);
			Angle	= RandomGen.randF(0, M_2PI);

			// Calcualte the new position.
			FoliagePosition.x += HypX * mCos(Angle);
			FoliagePosition.y += HypY * mSin(Angle);

			// Initialise RayCast Search Start/End Positions.
			FoliageStart = FoliageEnd = FoliagePosition;
			FoliageStart.z = 2000.f;
			FoliageEnd.z= -2000.f;

			// Perform Ray Cast Collision on Client.
			CollisionResult = gClientContainer.castRay(	FoliageStart, FoliageEnd, FXFOLIAGEREPLICATOR_COLLISION_MASK, &RayEvent);

			// Did we hit anything?
			if (CollisionResult)
			{
				// For now, let's pretend we didn't get a collision.
				CollisionResult = false;

				// Yes, so get it's type.
				U32 CollisionType = RayEvent.object->getTypeMask();

				// Check Illegal Placements, fail if we hit a disallowed type.
				if (((CollisionType & TerrainObjectType) && !mFieldData.mAllowOnTerrain)	||
					((CollisionType & InteriorObjectType) && !mFieldData.mAllowOnInteriors)	||
					((CollisionType & StaticTSObjectType) && !mFieldData.mAllowStatics)	||
					((CollisionType & WaterObjectType) && !mFieldData.mAllowOnWater) ) continue;

				// If we collided with water and are not allowing on the water surface then let's find the
				// terrain underneath and pass this on as the original collision else fail.
				if ((CollisionType & WaterObjectType) && !mFieldData.mAllowWaterSurface &&
					!gClientContainer.castRay( FoliageStart, FoliageEnd, FXFOLIAGEREPLICATOR_NOWATER_COLLISION_MASK, &RayEvent)) continue;

				// We passed with flying colour so carry on.
				CollisionResult = true;
			}

			// Invalidate if we are below Allowed Terrain Angle.
			if (RayEvent.normal.z < mSin(mDegToRad(90.0f-mFieldData.mAllowedTerrainSlope))) CollisionResult = false;

		// Wait until we get a collision.
		} while(!CollisionResult && --RelocationRetry);

		// Check for Relocation Problem.
		if (RelocationRetry > 0)
		{
			// Adjust Impact point.
			RayEvent.point.z += mFieldData.mOffsetZ;

			// Set New Position.
			FoliagePosition = RayEvent.point;
		}
		else
		{
			// Warning.
			Con::warnf(ConsoleLogEntry::General, "fxFoliageReplicator - Could not find satisfactory position for Foliage!");

			// Skip to next.
			continue;
		}

		// Monitor the total volume.
		FoliageOffsetPos = FoliagePosition - getPosition();
		MinPoint.setMin(FoliageOffsetPos);
		MaxPoint.setMax(FoliageOffsetPos);

		// Create our Foliage Item.
		pFoliageItem = new fxFoliageItem;

		// Reset Frame Serial.
		pFoliageItem->LastFrameSerialID = 0;

		// Reset Transform.
		pFoliageItem->Transform.identity();

		// Set Position.
		pFoliageItem->Transform.setColumn(3, FoliagePosition);
	
		// Are we fixing size @ max?
		if (mFieldData.mFixSizeToMax)
		{
			// Yes, so set height maximum height.
			pFoliageItem->Height = mFieldData.mMaxHeight;
			// Is the Aspect Ratio Fixed?
			if (mFieldData.mFixAspectRatio)
				// Yes, so lock to height.
				pFoliageItem->Width = pFoliageItem->Height;
			else
				// No, so set width to maximum width.
				pFoliageItem->Width = mFieldData.mMaxWidth;
		}
		else
		{
			// No, so choose a new Scale.
			pFoliageItem->Height = RandomGen.randF(mFieldData.mMinHeight, mFieldData.mMaxHeight);
			// Is the Aspect Ratio Fixed?
			if (mFieldData.mFixAspectRatio)
				// Yes, so lock to height.
				pFoliageItem->Width = pFoliageItem->Height;
			else
				// No, so choose a random width.
				pFoliageItem->Width = RandomGen.randF(mFieldData.mMinWidth, mFieldData.mMaxWidth);
		}

		// Are we randomly flipping horizontally?
		if (mFieldData.mRandomFlip)
			// Yes, so choose a random flip for this object.
			pFoliageItem->Flipped = (RandomGen.randF(0, 1000) < 500.0f) ? false : true;
		else
			// No, so turn-off flipping.
			pFoliageItem->Flipped = false;


		// Calculate Foliage Item World Box.
		// NOTE:-	We generate a psuedo-volume here.  It's basically the volume to which the
		//			plane can move and this includes swaying!
		//
		// Is Sway On?
		if (mFieldData.mSwayOn)
		{
			// Yes, so take swaying into account...
			pFoliageItem->FoliageBox.min =	FoliagePosition +
											Point3F(-pFoliageItem->Width / 2.0f - mFieldData.mSwayMagnitudeSide,
													-0.5f - mFieldData.mSwayMagnitudeFront,
													pFoliageItem->Height );

			pFoliageItem->FoliageBox.max =	FoliagePosition +
											Point3F(+pFoliageItem->Width / 2.0f + mFieldData.mSwayMagnitudeSide,
													+0.5f + mFieldData.mSwayMagnitudeFront,
													pFoliageItem->Height );
		}
		else
		{
			// No, so give it a minimum volume...
			pFoliageItem->FoliageBox.min =	FoliagePosition +
											Point3F(-pFoliageItem->Width / 2.0f,
													-0.5f,
													pFoliageItem->Height );

			pFoliageItem->FoliageBox.max =	FoliagePosition +
											Point3F(+pFoliageItem->Width / 2.0f,
													+0.5f,
													pFoliageItem->Height );
		}

		// Store Shape in Replicated Shapes Vector.
		mReplicatedFoliage.push_back(pFoliageItem);

		// Increase Foliage Count.
		mCurrentFoliageCount++;
	}

	// Is Lighting On?
	if (mFieldData.mLightOn)
	{
		// Yes, so step through Foliage.
		for (U32 idx = 0; idx < mCurrentFoliageCount; idx++)
		{
			fxFoliageItem*	pFoliageItem;

			// Fetch the Foliage Item.
			pFoliageItem = mReplicatedFoliage[idx];

			// Do we have an item?
			if (pFoliageItem)
			{
				// Yes, so are lights syncronised?
				if (mFieldData.mLightSync)
				{
					// Yes, so reset Global Light phase.
					mGlobalLightPhase = 0.0f;
					// Set Global Light Time Ratio.
					mGlobalLightTimeRatio = 719.0f / mFieldData.mLightTime;
				}
				else
				{
					// No, so choose a random Light phase.
					pFoliageItem->LightPhase = RandomGen.randF(0, 719.0f);
					// Set Light Time Ratio.
					pFoliageItem->LightTimeRatio = 719.0f / mFieldData.mLightTime;
				}
			}
		}

	}

	// Is Swaying Enabled?
	if (mFieldData.mSwayOn)
	{
		// Yes, so step through Foliage.
		for (U32 idx = 0; idx < mCurrentFoliageCount; idx++)
		{
			fxFoliageItem*	pFoliageItem;

			// Fetch the Foliage Item.
			pFoliageItem = mReplicatedFoliage[idx];

			// Do we have an item?
			if (pFoliageItem)
			{
				// Are we using Sway Sync?
				if (mFieldData.mSwaySync)
				{
					// Yes, so reset Global Sway phase.
					mGlobalSwayPhase = 0.0f;
					// Set Global Sway Time Ratio.
					mGlobalSwayTimeRatio = 719.0f / RandomGen.randF(mFieldData.mMinSwayTime, mFieldData.mMaxSwayTime);
				}
				else
				{
					// No, so choose a random Sway phase.
					pFoliageItem->SwayPhase = RandomGen.randF(0, 719.0f);
					// Set to random Sway Time.
					pFoliageItem->SwayTimeRatio = 719.0f / RandomGen.randF(mFieldData.mMinSwayTime, mFieldData.mMaxSwayTime);
				}
			}
		}
	}

	// Update our Object Volume.
	mObjBox.min.set(MinPoint);
	mObjBox.max.set(MaxPoint);
	setTransform(mObjToWorld);

	// ----------------------------------------------------------------------------------------------------------------------
	// Step 3.
	// ----------------------------------------------------------------------------------------------------------------------

	// Reset Next Allocated Node to Stack base.
	mNextAllocatedNodeIdx = 0;

	// Allocate a new Node.
	fxFoliageQuadrantNode* pNewNode = new fxFoliageQuadrantNode;

	// Store it in the Quad-tree.
	mFoliageQuadTree.push_back(pNewNode);

	// Populate Initial Node.
	//
	// Set Start Level.
	pNewNode->Level = mQuadTreeLevels;
	// Calculate Total Foliage Area.
	pNewNode->QuadrantBox = getWorldBox();
	// Reset Quadrant child nodes.
	pNewNode->QuadrantChildNode[0] =
	pNewNode->QuadrantChildNode[1] =
	pNewNode->QuadrantChildNode[2] =
	pNewNode->QuadrantChildNode[3] = NULL;

	// Create our initial cull list with *all* billboards into.
	fxFoliageCulledList CullList;
	CullList.mCulledObjectSet = mReplicatedFoliage;

	// Move to next node Index.
	mNextAllocatedNodeIdx++;

	// Let's start this thing going by recursing it's children.
	ProcessNodeChildren(pNewNode, &CullList);

	// Calculate Elapsed Time and take new Timestamp.
	F32 ElapsedTime = (Platform::getRealMilliseconds() - mStartCreationTime) * 0.001f;

	// Console Output.
	Con::printf("fxFoliageReplicator - Lev: %d  PotNodes: %d  Used: %d  Objs: %d  Time: %0.4fs.",
				mQuadTreeLevels,
				mPotentialFoliageNodes,
				mNextAllocatedNodeIdx-1,
				mBillboardsAcquired,
				ElapsedTime);

	// Dump (*very*) approximate allocated memory.
	F32 MemoryAllocated = (mNextAllocatedNodeIdx-1) * sizeof(fxFoliageQuadrantNode);
	MemoryAllocated		+=	mCurrentFoliageCount * sizeof(fxFoliageItem);
	MemoryAllocated		+=	mCurrentFoliageCount * sizeof(fxFoliageItem*);
	Con::printf("fxFoliageReplicator - Approx. %0.2fMb allocated.", MemoryAllocated / 1048576.0f);

	// ----------------------------------------------------------------------------------------------------------------------

	// Take first Timestamp.
	mLastRenderTime = Platform::getVirtualMilliseconds();
}

//------------------------------------------------------------------------------

Box3F fxFoliageReplicator::FetchQuadrant(Box3F Box, U32 Quadrant)
{
	Box3F QuadrantBox;

	// Select Quadrant.
	switch(Quadrant)
	{
		// UL.
		case 0:
			QuadrantBox.min = Box.min + Point3F(0, Box.len_y()/2, 0);
			QuadrantBox.max = QuadrantBox.min + Point3F(Box.len_x()/2, Box.len_y()/2, Box.len_z());
			break;

		// UR.
		case 1:
			QuadrantBox.min = Box.min + Point3F(Box.len_x()/2, Box.len_y()/2, 0);
			QuadrantBox.max = QuadrantBox.min + Point3F(Box.len_x()/2, Box.len_y()/2, Box.len_z());
			break;

		// LL.
		case 2:
			QuadrantBox.min = Box.min;
			QuadrantBox.max = QuadrantBox.min + Point3F(Box.len_x()/2, Box.len_y()/2, Box.len_z());
			break;

		// LR.
		case 3:
			QuadrantBox.min = Box.min + Point3F(Box.len_x()/2, 0, 0);
			QuadrantBox.max = QuadrantBox.min + Point3F(Box.len_x()/2, Box.len_y()/2, Box.len_z());
			break;

		default:
			return Box;
	}

	return QuadrantBox;
}

//------------------------------------------------------------------------------

void fxFoliageReplicator::ProcessNodeChildren(fxFoliageQuadrantNode* pParentNode, fxFoliageCulledList* pCullList)
{
	// ---------------------------------------------------------------
	// Split Node into Quadrants and Process each.
	// ---------------------------------------------------------------

	// Process All Quadrants (UL/UR/LL/LR).
	for (U32 q = 0; q < 4; q++)
		ProcessQuadrant(pParentNode, pCullList, q);
}

//------------------------------------------------------------------------------

void fxFoliageReplicator::ProcessQuadrant(fxFoliageQuadrantNode* pParentNode, fxFoliageCulledList* pCullList, U32 Quadrant)
{
	// Fetch Quadrant Box.
	const Box3F QuadrantBox = FetchQuadrant(pParentNode->QuadrantBox, Quadrant);

	// Create our new Cull List.
	fxFoliageCulledList CullList(QuadrantBox, pCullList);

	// Did we get any objects?
	if (CullList.GetListCount() > 0)
	{
		// Yes, so allocate a new Node.
		fxFoliageQuadrantNode* pNewNode = new fxFoliageQuadrantNode;

		// Store it in the Quad-tree.
		mFoliageQuadTree.push_back(pNewNode);

		// Move to next node Index.
		mNextAllocatedNodeIdx++;

		// Populate Quadrant Node.
		//
		// Next Sub-level.
		pNewNode->Level = pParentNode->Level - 1;
		// Calculate Quadrant Box.
		pNewNode->QuadrantBox = QuadrantBox;
		// Reset Child Nodes.
		pNewNode->QuadrantChildNode[0] =
		pNewNode->QuadrantChildNode[1] =
		pNewNode->QuadrantChildNode[2] =
		pNewNode->QuadrantChildNode[3] = NULL;
		
		// Put a reference in parent.
		pParentNode->QuadrantChildNode[Quadrant] = pNewNode;

		// If we're not at sub-level 0 then process this nodes children.
		if (pNewNode->Level != 0) ProcessNodeChildren(pNewNode, &CullList);
		// If we've reached sub-level 0 then store Cull List (for rendering).
		if (pNewNode->Level == 0)
		{
			// Store the render list from our culled object set.
			pNewNode->RenderList = CullList.mCulledObjectSet;
			// Keep track of the total billboard acquired.
			mBillboardsAcquired += CullList.GetListCount();
		}
	}
}

//------------------------------------------------------------------------------

void fxFoliageReplicator::SyncFoliageReplicators(void)
{
	// Check Host.
	AssertFatal(isServerObject(), "We *MUST* be on server when Synchronising Foliage!")

	// Find the Replicator Set.
	SimSet *fxFoliageSet = dynamic_cast<SimSet*>(Sim::findObject("fxFoliageSet"));

	// Return if Error.
	if (!fxFoliageSet)
	{
		// Console Warning.
		Con::warnf("fxFoliageReplicator - Cannot locate the 'fxFoliageSet', this is bad!");
		// Return here.
		return;
	}

	// Parse Replication Object(s).
	for (SimSetIterator itr(fxFoliageSet); *itr; ++itr)
	{
		// Fetch the Replicator Object.
		fxFoliageReplicator* Replicator = static_cast<fxFoliageReplicator*>(*itr);
		// Set Foliage Replication Mask.
		if (Replicator->isServerObject())
		{
			Con::printf("fxFoliageReplicator - Restarting fxFoliageReplicator Object...");
			Replicator->setMaskBits(FoliageReplicationMask);
		}
	}

	// Info ...
	Con::printf("fxFoliageReplicator - Client Foliage Sync has completed.");
}


//------------------------------------------------------------------------------

void fxFoliageReplicator::DestroyFoliage(void)
{
	// Check Host.
	AssertFatal(isClientObject(), "Trying to destroy Foliage on Server, this is bad!")

	// Destroy Quad-tree.
	mPotentialFoliageNodes = 0;
	// Reset Billboards Acquired.
	mBillboardsAcquired = 0;

	// Finish if we didn't create any shapes.
	if (mCurrentFoliageCount == 0) return;

	// Remove shapes.
	for (U32 idx = 0; idx < mCurrentFoliageCount; idx++)
	{
		fxFoliageItem*	pFoliageItem;

		// Fetch the Foliage Item.
		pFoliageItem = mReplicatedFoliage[idx];

		// Delete Shape.
		if (pFoliageItem) delete pFoliageItem;
	}

	// Let's remove the Quad-Tree allocations.
	for (	Vector<fxFoliageQuadrantNode*>::iterator QuadNodeItr = mFoliageQuadTree.begin();
			QuadNodeItr != mFoliageQuadTree.end();
			QuadNodeItr++ )
		{
			// Remove the node.
			delete *QuadNodeItr;
		}

	// Clear the Foliage Quad-Tree Vector.
	mFoliageQuadTree.clear();

	// Clear the Replicated Foliage Vector.
	mReplicatedFoliage.clear();

	// Clear the Frustum Render Set Vector.
	mFrustumRenderSet.mVisObjectSet.clear();


	// Reset Foliage Count.
	mCurrentFoliageCount = 0;
}

//------------------------------------------------------------------------------

void fxFoliageReplicator::StartUp(void)
{
	// Flag, Client Replication Started.
	mClientReplicationStarted = true;

	// Create foliage on Client.
	if (isClientObject()) CreateFoliage();
}

//------------------------------------------------------------------------------

bool fxFoliageReplicator::onAdd()
{
	if(!Parent::onAdd()) return(false);

	// Add the Replicator to the Replicator Set.
	dynamic_cast<SimSet*>(Sim::findObject("fxFoliageSet"))->addObject(this);

	// Set Default Object Box.
	mObjBox.min.set( -0.5, -0.5, -0.5 );
	mObjBox.max.set(  0.5,  0.5,  0.5 );
	// Reset the World Box.
	resetWorldBox();
	// Set the Render Transform.
	setRenderTransform(mObjToWorld);

	// Add to Scene.
	addToScene();
	mAddedToScene = true;

	// Are we on the client?
    if ( isClientObject() )
	{
		// Yes, so load foliage texture.
		mFieldData.mFoliageTexture = TextureHandle( mFieldData.mFoliageFile,  MeshTexture );

		// If we are in the editor then we can manually startup replication.
		if (gEditingMission) mClientReplicationStarted = true;
	}

	// Return OK.
	return(true);
}

//------------------------------------------------------------------------------

void fxFoliageReplicator::onRemove()
{
	// Remove the Replicator from the Replicator Set.
	dynamic_cast<SimSet*>(Sim::findObject("fxFoliageSet"))->removeObject(this);

	// Remove from Scene.
	removeFromScene();
	mAddedToScene = false;

	// Are we on the Client?
	if (isClientObject())
	{
		// Yes, so destroy Foliage.
		DestroyFoliage();

		// Remove Texture.
		mFieldData.mFoliageTexture = NULL;
	}

	// Do Parent.
	Parent::onRemove();
}

//------------------------------------------------------------------------------

void fxFoliageReplicator::inspectPostApply()
{
	// Set Parent.
	Parent::inspectPostApply();

	// Set Foliage Replication Mask (this object only).
	setMaskBits(FoliageReplicationMask);
}

//------------------------------------------------------------------------------

void fxFoliageReplicator::onEditorEnable()
{
}

//------------------------------------------------------------------------------

void fxFoliageReplicator::onEditorDisable()
{
}

//------------------------------------------------------------------------------

ConsoleFunction(StartFoliageReplication, void, 1, 1, "StartFoliageReplication()")
{
	// Find the Replicator Set.
	SimSet *fxFoliageSet = dynamic_cast<SimSet*>(Sim::findObject("fxFoliageSet"));

	// Return if Error.
	if (!fxFoliageSet)
	{
		// Console Warning.
		Con::warnf("fxFoliageReplicator - Cannot locate the 'fxFoliageSet', this is bad!");
		// Return here.
		return;
	}

	// Parse Replication Object(s).
	for (SimSetIterator itr(fxFoliageSet); *itr; ++itr)
	{
		// Fetch the Replicator Object.
		fxFoliageReplicator* Replicator = static_cast<fxFoliageReplicator*>(*itr);
		// Start Client Objects Only.
		if (Replicator->isClientObject()) Replicator->StartUp();
	}

	// Info ...
	Con::printf("fxFoliageReplicator - Client Foliage Replication Startup is complete.");
}

//------------------------------------------------------------------------------

bool fxFoliageReplicator::prepRenderImage(SceneState* state, const U32 stateKey, const U32 startZone,
								const bool modifyBaseZoneState)
{
	// Return if last state.
	if (isLastState(state, stateKey)) return false;
	// Set Last State.
	setLastState(state, stateKey);

   // Is Object Rendered?
   if (state->isObjectRendered(this))
   {	   
		// Yes, so get a SceneRenderImage.
		SceneRenderImage* image = new SceneRenderImage;
		// Populate it.
		image->obj = this;
		image->sortType = SceneRenderImage::Point;
      image->isTranslucent = true;
      image->poly[0] = getPosition();
		// Insert it into the scene images.
		state->insertRenderImage(image);
   }

   return false;
}

//------------------------------------------------------------------------------

void fxFoliageReplicator::renderObject(SceneState* state, SceneRenderImage*)
{
	// Check we are in Canonical State.
	AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on entry");

	// Calculate Elapsed Time and take new Timestamp.
	S32 Time = Platform::getVirtualMilliseconds();
	F32 ElapsedTime = (Time - mLastRenderTime) * 0.001f;
	mLastRenderTime = Time;

	// Setup out the Projection Matrix/Viewport.
	RectI viewport;
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	dglGetViewport(&viewport);
	state->setupBaseProjection();

	// Draw Placement Area (if needed and Editing Mission).
	if (gEditingMission && mFieldData.mShowPlacementArea)
	{
		// Setup our rendering state.
		glPushMatrix();
		dglMultMatrix(&getTransform());
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

		// Do we need to draw the Inner Radius?
		if (mFieldData.mInnerRadiusX || mFieldData.mInnerRadiusY)
		{
			// Yes, so draw Inner Radius.
			glBegin(GL_TRIANGLE_STRIP);
			for (U32 Angle = mCreationAreaAngle; Angle < (mCreationAreaAngle+360); Angle++)
			{
				F32		XPos, YPos;

				// Calculate Position.
				XPos = mFieldData.mInnerRadiusX * mCos(mDegToRad(-(F32)Angle));
				YPos = mFieldData.mInnerRadiusY * mSin(mDegToRad(-(F32)Angle));

				// Set Colour.
				glColor4f(	mFieldData.mPlaceAreaColour.red,
							mFieldData.mPlaceAreaColour.green,
							mFieldData.mPlaceAreaColour.blue,
							AREA_ANIMATION_ARC * (Angle-mCreationAreaAngle));

				// Draw Arc Line.
				glVertex3f(	XPos, YPos, -(F32)mFieldData.mPlacementBandHeight/2.0f);
				glVertex3f(	XPos, YPos, +(F32)mFieldData.mPlacementBandHeight/2.0f);

			}
			glEnd();
		}

		// Do we need to draw the Outer Radius?
		if (mFieldData.mOuterRadiusX || mFieldData.mOuterRadiusY)
		{
			// Yes, so draw Outer Radius.
			glBegin(GL_TRIANGLE_STRIP);
			for (U32 Angle = mCreationAreaAngle; Angle < (mCreationAreaAngle+360); Angle++)
			{
				F32		XPos, YPos;

				// Calculate Position.
				XPos = mFieldData.mOuterRadiusX * mCos(mDegToRad(-(F32)Angle));
				YPos = mFieldData.mOuterRadiusY * mSin(mDegToRad(-(F32)Angle));

				// Set Colour.
				glColor4f(	mFieldData.mPlaceAreaColour.red,
							mFieldData.mPlaceAreaColour.green,
							mFieldData.mPlaceAreaColour.blue,
							AREA_ANIMATION_ARC * (Angle-mCreationAreaAngle));

				// Draw Arc Line.
				glVertex3f(	XPos, YPos, -(F32)mFieldData.mPlacementBandHeight/2.0f);
				glVertex3f(	XPos, YPos, +(F32)mFieldData.mPlacementBandHeight/2.0f);

			}
			glEnd();
		}

		// Restore rendering state.
		glDisable(GL_BLEND);
		glPopMatrix();

		// Animate Area Selection.
		mCreationAreaAngle = (U32)(mCreationAreaAngle + (1000 * ElapsedTime));
		mCreationAreaAngle = mCreationAreaAngle % 360;
	}

	// Draw Foliage.
	if (!mFieldData.mHideFoliage && mCurrentFoliageCount)
	{
		// Calculate some constants.
		const F32	ClippedViewDistance		= mFieldData.mViewDistance;
		const F32	MinimumViewDistance		= mFieldData.mViewClosest - mFieldData.mFadeOutRegion;
		const F32	MaximumViewDistance		= ClippedViewDistance + mFieldData.mFadeInRegion;
		const F32	LuminanceMidPoint		= (mFieldData.mMinLuminance + mFieldData.mMaxLuminance) / 2.0f;
		const F32	LuminanceMagnitude		= mFieldData.mMaxLuminance - LuminanceMidPoint;

		// Billboard Details.
		MatrixF			ModelView;
		Point4F			Position;
		const Point4F	XRotation(1,0,0,0);
		const Point4F	YRotation(0,1,0,0);
		Point4F			ZRotation;
		F32				LeftTexPos;
		F32				RightTexPos;

		// Sway Luminance.
		F32	Luminance = 1.0f;

		// Reset Sway Offsets.
		F32	SwayOffsetX = 0.0f;
		F32	SwayOffsetY = 0.0f;

		// Is Swaying On and *in* Sync?
		if (mFieldData.mSwayOn && mFieldData.mSwaySync)
		{
			// Yes, so calculate Global Sway Offset.
			SwayOffsetX = mFieldData.mSwayMagnitudeSide * mCosTable[(U32)mGlobalSwayPhase];
			SwayOffsetY = mFieldData.mSwayMagnitudeFront * mSinTable[(U32)mGlobalSwayPhase];

			// Animate Global Sway Phase (Modulus).
			mGlobalSwayPhase = mGlobalSwayPhase + (mGlobalSwayTimeRatio * ElapsedTime);
			if (mGlobalSwayPhase >= 720.0f) mGlobalSwayPhase -= 720.0f;
		}

		// Is Light On and *in* Sync?
		if (mFieldData.mLightOn && mFieldData.mLightSync)
		{
			// Yes, so calculate Global Light Luminance.
			Luminance = LuminanceMidPoint + LuminanceMagnitude * mCosTable[(U32)mGlobalLightPhase];

			// Animate Global Light Phase (Modulus).
			mGlobalLightPhase = mGlobalLightPhase + (mGlobalLightTimeRatio * ElapsedTime);
			if (mGlobalLightPhase >= 720.0f) mGlobalLightPhase -= 720.0f;
		}

		// Are we using culling?
		if (mFieldData.mUseCulling)
		{
			// Yes, so clear the Render Object Set.
			mFrustumRenderSet.mVisObjectSet.clear();

			// Calculate nearest Clipping Far-Plane.
			//
			// NOTE:-	Here we want the nearest plane to which we want to clip.
			//			This will be either the 'mViewDistance'/'mFadeInRegion' or the
			//			frustum FarPlane.
			F32 FarClipPlane = getMin((F32)state->getFarPlane(), mFieldData.mViewDistance + mFieldData.mFadeInRegion);

			// Setup the Clip-Planes.
			mFrustumRenderSet.SetupClipPlanes(state, FarClipPlane);

			if ( mFieldData.mUseDebugInfo)
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
			}

			// Compile the Visible Set.
			mFrustumRenderSet.CompileVisibleSet(mFoliageQuadTree[0], getRenderTransform(), mFieldData.mUseDebugInfo);

			if ( mFieldData.mUseDebugInfo)
			{
				// Restore rendering state.
				glDisable(GL_BLEND);
			}
		}
		else
		{
			// No, so clear the Render Object Set.
			mFrustumRenderSet.mVisObjectSet.clear();

			// Handle *all* objects ... potential eeek!
			mFrustumRenderSet.mVisObjectSet.merge(mReplicatedFoliage);
		}

		// Increase Frame Serial ID.
		mFrameSerialID++;

		// Reset Foliage Processed Counter.
		U32 FoliageProcessed = 0;

		// Only process if we have any trivially visible objects.
		if (mFrustumRenderSet.mVisObjectSet.size() > 0)
		{
			// Setup Render State.
			glEnable            ( GL_TEXTURE_2D );
			glBindTexture		( GL_TEXTURE_2D, mFieldData.mFoliageTexture.getGLName() );
			glEnable			( GL_BLEND );
			glBlendFunc			( GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA );
			glTexEnvi           ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
			glEnable			( GL_ALPHA_TEST );
			glEnable			( GL_CULL_FACE );
			glAlphaFunc			( GL_GREATER, mFieldData.mAlphaCutoff );

			// Step through Foliage.
			for (U32 idx = 0; idx < mFrustumRenderSet.mVisObjectSet.size(); idx++)
			{
				fxFoliageItem*	pFoliageItem;
				F32				Width, Height;
				F32				Distance;
				F32				ItemAlpha;
				F32				FogAlpha;
			
				// Fetch the Foliage Item.
				pFoliageItem = mFrustumRenderSet.mVisObjectSet[idx];

				// Check to see if we've already rendered this object.
				//
				// NOTE:-	We need to do this as objects can overlap over quad-tree boundaries and are
				//			therefore picked up.  We don't want to remove these duplicates otherwise the
				//			overlapping billboards would be contained within a single quad-block only
				//			and could possibly be culled even though they are visible in the adjacent
				//			quad-block.  In other words, billboards are *not* treated as a point but rather
				//			a psuedo volume which stops them popping-up when the central point is visible.
				//
				//			We also mark the billboard as rendered even if it results in being culled
				//			further on as this stops it being rendered further this frame.
				//
				// Has the item been rendered this frame?
				if (pFoliageItem->LastFrameSerialID != mFrameSerialID)
				{
					// No, so flag as rendered this frame.
					pFoliageItem->LastFrameSerialID = mFrameSerialID;

					// Increase Foliage Processed Counter.
					FoliageProcessed++;

					// Is Swaying On and *not* in Sync?
					if (mFieldData.mSwayOn && !mFieldData.mSwaySync)
					{
						// Yes, so calculate Sway Offset.
						SwayOffsetX = mFieldData.mSwayMagnitudeSide * mCosTable[(U32)pFoliageItem->SwayPhase];
						SwayOffsetY = mFieldData.mSwayMagnitudeFront * mSinTable[(U32)pFoliageItem->SwayPhase];

						// Animate Sway Phase (Modulus).
						pFoliageItem->SwayPhase = pFoliageItem->SwayPhase + (pFoliageItem->SwayTimeRatio * ElapsedTime);
						if (pFoliageItem->SwayPhase >= 720.0f) pFoliageItem->SwayPhase -= 720.0f;
					}

					// Is Light On and *not* in Sync?
					if (mFieldData.mLightOn && !mFieldData.mLightSync)
					{
						// Yes, so calculate Light Luminance.
						Luminance = LuminanceMidPoint + LuminanceMagnitude * mCosTable[(U32)pFoliageItem->LightPhase];

						// Animate Light Phase (Modulus).
						pFoliageItem->LightPhase = pFoliageItem->LightPhase + (pFoliageItem->LightTimeRatio * ElapsedTime);
						if (pFoliageItem->LightPhase >= 720.0f) pFoliageItem->LightPhase -= 720.0f;
					}

					// Calculate Distance to Item.
					Distance = (pFoliageItem->Transform.getPosition() - state->getCameraPosition()).len();

					// Trivially reject the billboard if it's beyond the SceneGraphs visible distance.
					if (Distance > state->getVisibleDistance())	continue;

					// Calculate Fog Alpha.
					FogAlpha = 1.0f - state->getHazeAndFog(Distance, pFoliageItem->Transform.getPosition().z - state->getCameraPosition().z);

					// Trivially reject the billboard if it's totally transparent.
					if (FogAlpha < FXFOLIAGE_ALPHA_EPSILON) continue;

					// Can we trivially accept the billboard?
					if (Distance >= MinimumViewDistance && Distance <= MaximumViewDistance)
					{
						// Yes, so are we fading out?
						if (Distance < mFieldData.mViewClosest)
						{
							// Yes, so set fade-out.
							ItemAlpha = 1.0f - ((mFieldData.mViewClosest - Distance) * mFadeOutGradient);
						}
						// No, so are we fading in?
						else if (Distance > ClippedViewDistance)
						{
							// Yes, so set fade-in
							ItemAlpha = 1.0f - ((Distance - ClippedViewDistance) * mFadeInGradient);
						}
						// No, so set full.
						else
						{
							ItemAlpha = 1.0f;
						}

						// Clamp upper-limit to Fog Alpha.
						if (ItemAlpha > FogAlpha) ItemAlpha = FogAlpha;

						// Store our Modelview.
						glPushMatrix();

						// Perform Spherical Billboarding.
						dglMultMatrix(&pFoliageItem->Transform);
						dglGetModelview(&ModelView);
						ModelView.setColumn(0, XRotation);
						ModelView.setColumn(1, YRotation);
						dglLoadMatrix(&ModelView);

						// Fetch Width/Height.
						Width	= pFoliageItem->Width / 2.0f;
						Height	= pFoliageItem->Height;

						// Fetch Flipped Flag.
						LeftTexPos	= pFoliageItem->Flipped ? 1.0f : 0.0f;
						RightTexPos	= 1.0f - LeftTexPos;

						// Draw Billboard.
						glBegin(GL_QUADS);
							// Set Blend Function.
							glColor4f(Luminance,Luminance,Luminance, ItemAlpha);

							// Draw Top part of billboard.
							glTexCoord2f	(LeftTexPos,0);
							glVertex3f		(-Width+SwayOffsetX,SwayOffsetY,Height);
							glTexCoord2f	(RightTexPos,0);
							glVertex3f		(+Width+SwayOffsetX,SwayOffsetY,Height);

							// Set Ground Blend.
							if (mFieldData.mGroundAlpha < 1.0f) glColor4f(Luminance, Luminance, Luminance, mFieldData.mGroundAlpha < ItemAlpha ? mFieldData.mGroundAlpha : ItemAlpha);

							// Draw bottom part of billboard.
							glTexCoord2f	(RightTexPos,1);
							glVertex3f		(+Width,0,0);
							glTexCoord2f	(LeftTexPos,1);
							glVertex3f		(-Width,0,0);
						glEnd();

						// Restore our Modelview.
						glPopMatrix();
					}
				}
			}

			// Restore rendering state.
			glTexEnvi			( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
			glDisable			( GL_CULL_FACE );
			glDisable			( GL_ALPHA_TEST );
			glDisable			( GL_BLEND );
			glDisable			( GL_TEXTURE_2D );
		}

		// Dump Debug Information (Uncomment this if needed).
		//
		// NOTE:-	Here we show the number of objects rendered compared to the potential objects in the viewing frustum.
		//			Remember that the difference is simply billboards that intersect quad-tree boundaries.
		//
		//if (mFieldData.mUseDebugInfo) Con::printf("fxFoliageReplicator - %d of %d Foliage Items Processed", FoliageProcessed, mFrustumRenderSet.mVisObjectSet.size());
	}

	// Restore out nice and friendly canonical state.
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	dglSetViewport(viewport);

	// Check we have restored Canonical State.
	AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on exit");
}

//------------------------------------------------------------------------------

U32 fxFoliageReplicator::packUpdate(NetConnection * con, U32 mask, BitStream * stream)
{
	// Pack Parent.
	U32 retMask = Parent::packUpdate(con, mask, stream);

	// Write Foliage Replication Flag.
	if (stream->writeFlag(mask & FoliageReplicationMask))
	{
		stream->writeAffineTransform(mObjToWorld);						// Foliage Master-Object Position.

		stream->writeFlag(mFieldData.mUseDebugInfo);					// Foliage Debug Information Flag.
		stream->write(mFieldData.mDebugBoxHeight);						// Foliage Debug Height.
		stream->write(mFieldData.mSeed);								// Foliage Seed.
		stream->write(mFieldData.mFoliageCount);						// Foliage Count.
		stream->write(mFieldData.mFoliageRetries);						// Foliage Retries.
		stream->writeString(mFieldData.mFoliageFile);					// Foliage File.

		stream->write(mFieldData.mInnerRadiusX);						// Foliage Inner Radius X.
		stream->write(mFieldData.mInnerRadiusY);						// Foliage Inner Radius Y.
		stream->write(mFieldData.mOuterRadiusX);						// Foliage Outer Radius X.
		stream->write(mFieldData.mOuterRadiusY);						// Foliage Outer Radius Y.

		stream->write(mFieldData.mMinWidth);							// Foliage Minimum Width.
		stream->write(mFieldData.mMaxWidth);							// Foliage Maximum Width.
		stream->write(mFieldData.mMinHeight);							// Foliage Minimum Height.
		stream->write(mFieldData.mMaxHeight);							// Foliage Maximum Height.
		stream->write(mFieldData.mFixAspectRatio);						// Foliage Fix Aspect Ratio.
		stream->write(mFieldData.mFixSizeToMax);						// Foliage Fix Size to Max.
		stream->write(mFieldData.mOffsetZ);								// Foliage Offset Z.
		stream->write(mFieldData.mRandomFlip);							// Foliage Random Flip.

		stream->write(mFieldData.mUseCulling);							// Foliage Use Culling.
		stream->write(mFieldData.mCullResolution);						// Foliage Cull Resolution.
		stream->write(mFieldData.mViewDistance);						// Foliage View Distance.
		stream->write(mFieldData.mViewClosest);							// Foliage View Closest.
		stream->write(mFieldData.mFadeInRegion);						// Foliage Fade-In Region.
		stream->write(mFieldData.mFadeOutRegion);						// Foliage Fade-Out Region.
		stream->write(mFieldData.mAlphaCutoff);							// Foliage Alpha Cutoff.
		stream->write(mFieldData.mGroundAlpha);							// Foliage Ground Alpha.

		stream->writeFlag(mFieldData.mSwayOn);							// Foliage Sway On Flag.
		stream->writeFlag(mFieldData.mSwaySync);						// Foliage Sway Sync Flag.
		stream->write(mFieldData.mSwayMagnitudeSide);					// Foliage Sway Magnitude Side2Side.
		stream->write(mFieldData.mSwayMagnitudeFront);					// Foliage Sway Magnitude Front2Back.
		stream->write(mFieldData.mMinSwayTime);							// Foliage Minimum Sway Time.
		stream->write(mFieldData.mMaxSwayTime);							// Foliage Maximum way Time.

		stream->writeFlag(mFieldData.mLightOn);							// Foliage Light On Flag.
		stream->writeFlag(mFieldData.mLightSync);						// Foliage Light Sync
		stream->write(mFieldData.mMinLuminance);						// Foliage Minimum Luminance.
		stream->write(mFieldData.mMaxLuminance);						// Foliage Maximum Luminance.
		stream->write(mFieldData.mLightTime);							// Foliage Light Time.

		stream->writeFlag(mFieldData.mAllowOnTerrain);					// Allow on Terrain.
		stream->writeFlag(mFieldData.mAllowOnInteriors);				// Allow on Interiors.
		stream->writeFlag(mFieldData.mAllowStatics);					// Allow on Statics.
		stream->writeFlag(mFieldData.mAllowOnWater);					// Allow on Water.
		stream->writeFlag(mFieldData.mAllowWaterSurface);				// Allow on Water Surface.
		stream->write(mFieldData.mAllowedTerrainSlope);					// Foliage Offset Z.

		stream->writeFlag(mFieldData.mHideFoliage);						// Hide Foliage.
		stream->writeFlag(mFieldData.mShowPlacementArea);				// Show Placement Area Flag.
		stream->write(mFieldData.mPlacementBandHeight);					// Placement Area Height.
		stream->write(mFieldData.mPlaceAreaColour);						// Placement Area Colour.
	}

	// Were done ...
	return(retMask);
}

//------------------------------------------------------------------------------

void fxFoliageReplicator::unpackUpdate(NetConnection * con, BitStream * stream)
{
	// Unpack Parent.
	Parent::unpackUpdate(con, stream);

	// Read Replication Details.
	if(stream->readFlag())
	{
		MatrixF		ReplicatorObjectMatrix;

		stream->readAffineTransform(&ReplicatorObjectMatrix);			// Foliage Master Object Position.

		mFieldData.mUseDebugInfo = stream->readFlag();					// Foliage Debug Information Flag.
		stream->read(&mFieldData.mDebugBoxHeight);						// Foliage Debug Height.
		stream->read(&mFieldData.mSeed);								// Foliage Seed.
		stream->read(&mFieldData.mFoliageCount);						// Foliage Count.
		stream->read(&mFieldData.mFoliageRetries);						// Foliage Retries.
		mFieldData.mFoliageFile = stream->readSTString();				// Foliage File.

		stream->read(&mFieldData.mInnerRadiusX);						// Foliage Inner Radius X.
		stream->read(&mFieldData.mInnerRadiusY);						// Foliage Inner Radius Y.
		stream->read(&mFieldData.mOuterRadiusX);						// Foliage Outer Radius X.
		stream->read(&mFieldData.mOuterRadiusY);						// Foliage Outer Radius Y.

		stream->read(&mFieldData.mMinWidth);							// Foliage Minimum Width.
		stream->read(&mFieldData.mMaxWidth);							// Foliage Maximum Width.
		stream->read(&mFieldData.mMinHeight);							// Foliage Minimum Height.
		stream->read(&mFieldData.mMaxHeight);							// Foliage Maximum Height.
		stream->read(&mFieldData.mFixAspectRatio);						// Foliage Fix Aspect Ratio.
		stream->read(&mFieldData.mFixSizeToMax);						// Foliage Fix Size to Max.
		stream->read(&mFieldData.mOffsetZ);								// Foliage Offset Z.
		stream->read(&mFieldData.mRandomFlip);							// Foliage Random Flip.

		stream->read(&mFieldData.mUseCulling);							// Foliage Use Culling.
		stream->read(&mFieldData.mCullResolution);						// Foliage Cull Resolution.
		stream->read(&mFieldData.mViewDistance);						// Foliage View Distance.
		stream->read(&mFieldData.mViewClosest);							// Foliage View Closest.
		stream->read(&mFieldData.mFadeInRegion);						// Foliage Fade-In Region.
		stream->read(&mFieldData.mFadeOutRegion);						// Foliage Fade-Out Region.
		stream->read(&mFieldData.mAlphaCutoff);							// Foliage Alpha Cutoff.
		stream->read(&mFieldData.mGroundAlpha);							// Foliage Ground Alpha.

		mFieldData.mSwayOn = stream->readFlag();						// Foliage Sway On Flag.
		mFieldData.mSwaySync = stream->readFlag();						// Foliage Sway Sync Flag.
		stream->read(&mFieldData.mSwayMagnitudeSide);					// Foliage Sway Magnitude Side2Side.
		stream->read(&mFieldData.mSwayMagnitudeFront);					// Foliage Sway Magnitude Front2Back.
		stream->read(&mFieldData.mMinSwayTime);							// Foliage Minimum Sway Time.
		stream->read(&mFieldData.mMaxSwayTime);							// Foliage Maximum way Time.

		mFieldData.mLightOn = stream->readFlag();						// Foliage Light On Flag.
		mFieldData.mLightSync = stream->readFlag();						// Foliage Light Sync
		stream->read(&mFieldData.mMinLuminance);						// Foliage Minimum Luminance.
		stream->read(&mFieldData.mMaxLuminance);						// Foliage Maximum Luminance.
		stream->read(&mFieldData.mLightTime);							// Foliage Light Time.

		mFieldData.mAllowOnTerrain = stream->readFlag();				// Allow on Terrain.
		mFieldData.mAllowOnInteriors = stream->readFlag();				// Allow on Interiors.
		mFieldData.mAllowStatics = stream->readFlag();					// Allow on Statics.
		mFieldData.mAllowOnWater = stream->readFlag();					// Allow on Water.
		mFieldData.mAllowWaterSurface = stream->readFlag();				// Allow on Water Surface.
		stream->read(&mFieldData.mAllowedTerrainSlope);					// Allowed Terrain Slope.

		mFieldData.mHideFoliage = stream->readFlag();					// Hide Foliage.
		mFieldData.mShowPlacementArea = stream->readFlag();				// Show Placement Area Flag.
		stream->read(&mFieldData.mPlacementBandHeight);					// Placement Area Height.
		stream->read(&mFieldData.mPlaceAreaColour);

		// Calculate Fade-In/Out Gradients.
		mFadeInGradient		= 1.0f / mFieldData.mFadeInRegion;
		mFadeOutGradient	= 1.0f / mFieldData.mFadeOutRegion;

		// Set Transform.
		setTransform(ReplicatorObjectMatrix);

		// Load Foliage Texture on the client.
		mFieldData.mFoliageTexture = TextureHandle( mFieldData.mFoliageFile,  MeshTexture );

		// Set Quad-Tree Box Height Lerp.
		mFrustumRenderSet.mHeightLerp = mFieldData.mDebugBoxHeight;

		// Create Foliage (if Replication has begun).
		if (mClientReplicationStarted) CreateFoliage();
	}
}

