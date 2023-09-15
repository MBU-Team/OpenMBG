//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "dgl/dgl.h"
#include "gui/guiBitmapCtrl.h"
#include "console/consoleTypes.h"
#include "game/gameConnection.h"
#include "game/vehicles/vehicle.h"

//-----------------------------------------------------------------------------
/// A Speedometer control.
/// This gui displays the speed of the current Vehicle based
/// control object. This control only works if a server
/// connection exists and it's control object is a vehicle. If
/// either of these requirements is false, the control is not rendered.
class GuiSpeedometerHud : public GuiBitmapCtrl
{
   typedef GuiBitmapCtrl Parent;

   F32   mSpeed;        ///< Current speed
   F32   mMaxSpeed;     ///< Max speed at max need pos
   F32   mMaxAngle;     ///< Max pos of needle
   F32   mMinAngle;     ///< Min pos of needle
   Point2F mCenter;     ///< Center of needle rotation
   ColorF mColor;       ///< Needle Color
   F32   mNeedleLength;
   F32   mNeedleWidth;
   F32   mTailLength;

public:
   GuiSpeedometerHud();

   void onRender( Point2I, const RectI &);
   static void initPersistFields();
   DECLARE_CONOBJECT( GuiSpeedometerHud );
};


//-----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT( GuiSpeedometerHud );

GuiSpeedometerHud::GuiSpeedometerHud()
{
   mSpeed = 0;
   mMaxSpeed = 100;
   mMaxAngle = 0;
   mMinAngle = 200;
   mCenter.set(0,0);
   mNeedleWidth = 3;
   mNeedleLength = 10;
   mTailLength = 5;
   mColor.set(1,0,0,1);
}

void GuiSpeedometerHud::initPersistFields()
{
   Parent::initPersistFields();

   addGroup("Needle");
   addField("maxSpeed", TypeF32, Offset( mMaxSpeed, GuiSpeedometerHud ) );
   addField("minAngle", TypeF32, Offset( mMinAngle, GuiSpeedometerHud ) );
   addField("maxAngle", TypeF32, Offset( mMaxAngle, GuiSpeedometerHud ) );
   addField("color", TypeColorF, Offset( mColor, GuiSpeedometerHud ) );
   addField("center", TypePoint2F, Offset( mCenter, GuiSpeedometerHud ) );
   addField("length", TypeF32, Offset( mNeedleLength, GuiSpeedometerHud ) );
   addField("width", TypeF32, Offset( mNeedleWidth, GuiSpeedometerHud ) );
   addField("tail", TypeF32, Offset( mTailLength, GuiSpeedometerHud ) );
   endGroup("Needle");
}


//-----------------------------------------------------------------------------
/**
   Gui onRender method.
   Renders a health bar with filled background and border.
*/
void GuiSpeedometerHud::onRender(Point2I offset, const RectI &updateRect)
{
   // Must have a connection and player control object
   GameConnection* conn = GameConnection::getServerConnection();
   if (!conn)
      return;
   Vehicle* control = dynamic_cast<Vehicle*>(conn->getControlObject());
   if (!control)
      return;

   Parent::onRender(offset,updateRect);
   
   // Use the vehicle's velocity as it's speed...
   mSpeed = control->getVelocity().len();
   if (mSpeed > mMaxSpeed)
      mSpeed = mMaxSpeed;
   
   // Render the needle
   glPushMatrix();
   Point2F center = mCenter;
   if (center.x == F32(0) && center.y == F32(0)) {
      center.x = mBounds.extent.x / 2;
      center.y = mBounds.extent.y / 2;
   }
   glTranslatef(mBounds.point.x + center.x,mBounds.point.y + center.y,0);

   F32 rotation = mMinAngle + (mMaxAngle - mMinAngle) * (mSpeed / mMaxSpeed);
   glRotatef(rotation,0.0f,0.0f,-1.0f);

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glDisable(GL_TEXTURE_2D);

   glColor4f(mColor.red, mColor.green, mColor.blue, mColor.alpha);
   glBegin(GL_LINE_LOOP);
      glVertex2f(+mNeedleLength,-mNeedleWidth);
      glVertex2f(+mNeedleLength,+mNeedleWidth);
      glVertex2f(-mTailLength ,+mNeedleWidth);
      glVertex2f(-mTailLength ,-mNeedleWidth);
   glEnd();
   glPopMatrix();
}


