//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GAMETSCTRL_H_
#define _GAMETSCTRL_H_

#ifndef _DGL_H_
#include "dgl/dgl.h"
#endif
#ifndef _GAME_H_
#include "game/game.h"
#endif
#ifndef _GUITSCONTROL_H_
#include "gui/guiTSControl.h"
#endif

class ProjectileData;
class GameBase;

//----------------------------------------------------------------------------
class GameTSCtrl : public GuiTSCtrl
{
private:
   typedef GuiTSCtrl Parent;

public:
   GameTSCtrl();

   bool processCameraQuery(CameraQuery *query);
   void renderWorld(const RectI &updateRect);

   void onMouseMove(const GuiEvent &evt);
   void onRender(Point2I offset, const RectI &updateRect);
 
   DECLARE_CONOBJECT(GameTSCtrl);
};

#endif
