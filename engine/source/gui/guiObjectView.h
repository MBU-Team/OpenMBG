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

#ifndef _GUIOBJECTVIEW_H_
#define _GUIOBJECTVIEW_H_

#ifndef _GUITSCONTROL_H_
#include "gui/guiTSControl.h"
#endif
#ifndef _TSSHAPEINSTANCE_H_
#include "ts/tsShapeInstance.h"
#endif
#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
#endif
#ifndef _CAMERA_H_
#include "game/camera.h"
#endif

class TSThread;

class GuiObjectView : public GuiTSCtrl
{
	private:
		typedef GuiTSCtrl Parent;

	protected:
		enum MouseState
		{
			None,
			Rotating,
			Zooming
		};
		
		MouseState  mMouseState;

		TSShapeInstance* mModel;
		
		bool mZoom;
		bool mSpin;

		Point3F  mCameraPos;
		MatrixF  mCameraMatrix;
		EulerF   mCameraRot;
		EulerF   mCameraRotSpeed;
		EulerF   mCameraOffset;
		Point3F  mOrbitPos;
		F32      mMinOrbitDist;
		F32      mOrbitDist;

		Point2I  mLastMousePoint;
		S32 lastRenderTime;

		Point3F     mLightPos;
		VectorF     mLightDirection;
		ColorF      mLightColor;
		ColorF      mLightAmbient;
		
		StringTableEntry mModelName;
		StringTableEntry mSkinName;

	public:
		DECLARE_CONOBJECT( GuiObjectView );
		GuiObjectView();
		~GuiObjectView();

		static void consoleInit();
		static void initPersistFields();
		bool onWake();

		void onMouseDown( const GuiEvent &event );
		void onMouseUp( const GuiEvent &event );
		void onMouseDragged( const GuiEvent &event );
		void onRightMouseDown( const GuiEvent &event );
		void onRightMouseUp( const GuiEvent &event );
		void onRightMouseDragged( const GuiEvent &event );

		bool processCameraQuery( CameraQuery *query );
		void renderWorld( const RectI &updateRect );
		void setObjectModel(const char* modelName, const char* skinName);
};

#endif // _GUIOBJECTVIEW_H_