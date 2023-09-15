//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (c) 2002 GarageGames.Com
//-----------------------------------------------------------------------------

#ifndef _H_GUIDEFAULTCONTROLRENDER_
#define _H_GUIDEFAULTCONTROLRENDER_

class GuiControlProfile;

void renderRaisedBox(RectI &bounds, GuiControlProfile *profile);
void renderSlightlyRaisedBox(RectI &bounds, GuiControlProfile *profile);
void renderLoweredBox(RectI &bounds, GuiControlProfile *profile);
void renderSlightlyLoweredBox(RectI &bounds, GuiControlProfile *profile);
void renderBorder(RectI &bounds, GuiControlProfile *profile);

#endif
