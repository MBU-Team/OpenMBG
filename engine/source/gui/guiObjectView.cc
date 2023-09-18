//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2003 Black Blood Studios
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Sierra Online, Inc.
//-----------------------------------------------------------------------------
// This release by: Xavier Amado (xavier@blackbloodstudios.com)
// Credits to : Vinci_Smurf, Loonatik, Matt Webster, Frank Bignone, Xavier Amado
//-----------------------------------------------------------------------------

#include "gui/guiCanvas.h"
#include "gui/guiObjectView.h"
#include "console/consoleTypes.h"

static const F32 MaxOrbitDist = 50.0f;
static const S32 MaxAnimations = 6;

IMPLEMENT_CONOBJECT( GuiObjectView );


GuiObjectView::GuiObjectView() : GuiTSCtrl()
{
	mActive = true;

	mMouseState = None;

	// Can zoom and spin by default
	mZoom = true;
	mSpin = true;
	mLastMousePoint.set( 0, 0 );

	mModel = NULL;
	
	mLightPos = VectorF(1.0f, 1.0f, 1.0f);
	mLightDirection = VectorF(-0.57735f, -0.57735f, -0.57735f);
	mLightColor = ColorF(0.6f, 0.58f, 0.5f);
	mLightAmbient = ColorF(0.3f, 0.3f, 0.3f);

	mCameraMatrix.identity();
	mCameraRot.set(0.0f, 0.0f, 0.0f);//3.9f);
	mCameraRotSpeed.set(0.0f, 0.0f, 0.001f);
	mCameraPos.set(0.0f, 1.75f, 1.25f);
	mCameraMatrix.setColumn(3, mCameraPos);
	mOrbitPos.set(0.0f, 0.0f, 0.0f);

	mModelName = NULL;
	mSkinName = StringTable->insert("base");

	lastRenderTime = 0;
	mMinOrbitDist = 0.0f;
	mOrbitDist = 9.5f;
}

GuiObjectView::~GuiObjectView()
{
	if (mModel != NULL) {
		delete mModel;
		mModel = NULL;
	}
}

void GuiObjectView::consoleInit()
{

}

void GuiObjectView::initPersistFields()
{
	Parent::initPersistFields();
	addGroup("Object View");		// MM: Added Group Header.
	addField("model", TypeFilename, Offset(mModelName, GuiObjectView));
	addField("skin", TypeString, Offset(mSkinName, GuiObjectView));
	addField("cameraRotX", TypeF32, Offset(mCameraRot.x, GuiObjectView));
	addField("cameraZRotSpeed", TypeF32, Offset(mCameraRotSpeed.z, GuiObjectView));
	addField("orbitDistance", TypeF32, Offset(mOrbitDist, GuiObjectView));
	endGroup("Object View");		// MM: Added Group Footer.
}

bool GuiObjectView::onWake()
{
	if ( !Parent::onWake() )
		return( false );

	if (mModelName && mModelName[0])
		setObjectModel(mModelName, mSkinName);

	return( true );
}

// Mouse is down, lock mouse input and get the mouse pointer coordinates and set mode to spin
void GuiObjectView::onMouseDown( const GuiEvent &event )
{
	if ( !mActive || !mVisible || !mAwake || !mSpin )
		return;

	mMouseState = Rotating;

	mLastMousePoint = event.mousePoint;
	mouseLock();
}

// Mouse is up, unlock mouse input
void GuiObjectView::onMouseUp( const GuiEvent &/*event*/ )
{
	mouseUnlock();
	mMouseState = None;
}

// If mouse is dragged, adjust camera position accordingly. Makes model rotate
void GuiObjectView::onMouseDragged( const GuiEvent &event )
{
	if ( mMouseState != Rotating )
		return;

	Point2I delta = event.mousePoint - mLastMousePoint;
	mLastMousePoint = event.mousePoint;

	mCameraRot.x += ( delta.y * 0.01 );
	mCameraRot.z += ( delta.x * 0.01 );
}

// Right mouse is down, lock mouse input and get the mouse pointer coordinates and set mode to zoom
void GuiObjectView::onRightMouseDown( const GuiEvent &event )
{
	if ( !mActive || !mVisible || !mAwake || !mZoom )
		return;

	mMouseState = Zooming;

	mLastMousePoint = event.mousePoint;
	mouseLock();
}

// Right mouse is up, unlock mouse input
void GuiObjectView::onRightMouseUp( const GuiEvent &/*event*/ )
{
	mouseUnlock();
	mMouseState = None;
}

// If mouse is dragged, adjust camera position accordingly. Makes model zoom
void GuiObjectView::onRightMouseDragged( const GuiEvent &event )
{
	if ( mMouseState != Zooming )
		return;

	S32 delta = event.mousePoint.y - mLastMousePoint.y;
	mLastMousePoint = event.mousePoint;

	mOrbitDist += ( delta * 0.01 ); 
}


//------------------------------------------------------------------------------
bool GuiObjectView::processCameraQuery( CameraQuery* query )
{
	// Adjust the camera so that we are still facing the model:
	Point3F vec;
	MatrixF xRot, zRot;
	xRot.set(EulerF(mCameraRot.x, 0.0f, 0.0f));
	zRot.set(EulerF(0.0f, 0.0f, mCameraRot.z));

	mCameraMatrix.mul(zRot, xRot);
	mCameraMatrix.getColumn(1, &vec);
	vec *= mOrbitDist;
	mCameraPos = mOrbitPos - vec;

	query->farPlane = 2100.0f;
	query->nearPlane = query->farPlane / 5000.0f;
	query->fov = 45.0f;
	mCameraMatrix.setColumn(3, mCameraPos);
	query->cameraMatrix = mCameraMatrix;
	return( true );
}


//------------------------------------------------------------------------------
void GuiObjectView::renderWorld( const RectI &updateRect )
{
	S32 time = Platform::getVirtualMilliseconds();
	S32 dt = time - lastRenderTime;
	lastRenderTime = time;

	glClear( GL_DEPTH_BUFFER_BIT );
	glMatrixMode( GL_MODELVIEW );

	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );

	if (mModel) {
		mCameraRot += mCameraRotSpeed * dt;
		mModel->render();
	}

	glDisable( GL_DEPTH_TEST );
	dglSetClipRect( updateRect );
	dglSetCanonicalState();
}

void GuiObjectView::setObjectModel(const char* modelName, const char* skinName)
{
	if (mModel != NULL) {
		delete mModel;
		mModel = NULL;
	}

	if (!modelName)
		return;

	Resource<TSShape> model = ResourceManager->load(modelName);
	if (!bool(model))
	{
		Con::warnf(avar("GuiObjectView: Failed to load model %s. Please check your model name and load a valid model.",
			modelName));
		return;
	}

	mModel = new TSShapeInstance(model, true);
	AssertFatal(mModel,
		avar("GuiObjectView: Failed to load model %s. Please check your model name and load a valid model.",
			modelName));

	if (dStrcmp(skinName, "base") != 0)
	{
		StringHandle skinHandle = StringHandle(skinName);
		mModel->reSkin(skinHandle);
	}

	// Initialize camera values:
	mOrbitPos = mModel->getShape()->center;
	mMinOrbitDist = mModel->getShape()->radius;

}

ConsoleMethod(GuiObjectView, setModel, void, 4, 4,
	"(string shapeName)\n"
	"(string skinName)\n"
	"Sets the model to be displayed in this control\n\n"
	"\\param shapeName Name of the model to display.\n"
	"\\param skinName Name of the skin to use for the model.\n")
{
	argc;
	object->setObjectModel(argv[2], argv[3]);
}

ConsoleMethod(GuiObjectView, setCameraDist, void, 3, 3,
	"(float distance)\n"
	"Sets the distance at which the camera orbits the object. Clamped to the acceptable range defined in the class by min and max orbit distances.\n\n"
	"\\param distance The distance to set the orbit to (will be clamped).")
{
	argc;
	object->setCameraDistance(dAtof(argv[2]));
}

ConsoleMethod(GuiObjectView, setCameraPitch, void, 3, 3, "objectView.setCameraPitch( speed )")
{
	argc;
	object->setCameraPitch(dAtof(argv[2]));
}

ConsoleMethod(GuiObjectView, setCameraRotSpeed, void, 3, 3, "objectView.setCameraRotSpeed( speed )")
{
	argc;
	object->setCameraRotSpeed(dAtof(argv[2]));
}

ConsoleMethod(GuiObjectView, setEmpty, void, 2, 2,
	"()\n"
	"Removes the model displayed in this control\n\n")
{
	argc;
	object->setEmpty();
}
