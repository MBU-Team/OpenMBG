//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------


#include "game/cameraSpline.h"

#include "console/console.h"
#include "platform/platformGL.h"

//-----------------------------------------------------------------------------

CameraSpline::Knot::Knot(const Knot &k)
{
   mPosition = k.mPosition; 
   mRotation = k.mRotation; 
   mSpeed    = k.mSpeed;
   mType = k.mType;
   mPath = k.mPath;
   prev = NULL; next = NULL; 
}   

CameraSpline::Knot::Knot(const Point3F &p, const QuatF &r, F32 s, Knot::Type type, Knot::Path path) 
{ 
   mPosition = p; 
   mRotation = r; 
   mSpeed    = s;
   mType = type;
   mPath = path;
   prev = NULL; next = NULL; 
} 


//-----------------------------------------------------------------------------

CameraSpline::CameraSpline()
{
   mFront = NULL; 
   mSize = 0;
   mIsMapDirty = true;
   VECTOR_SET_ASSOCIATION(mTimeMap);
}   


CameraSpline::~CameraSpline()
{
   removeAll();
}


void CameraSpline::push_back(Knot *w)
{
   if (!mFront)
   {
      mFront = w;
      w->next = w;
      w->prev = w;
   }
   else
   {
      Knot *before = back();
      Knot *after  = before->next;

      w->next = before->next;
      w->prev = before;
      after->prev = w;
      before->next = w;
   }
   ++mSize;
   mIsMapDirty = true;
}   

CameraSpline::Knot* CameraSpline::getKnot(S32 i)
{
   Knot *k = mFront;
   while(i--)
      k = k->next;
   return k;
}

CameraSpline::Knot* CameraSpline::remove(Knot *w)
{
   if (w->next == mFront && w->prev == mFront)
      mFront = NULL;
   else
   {
      w->prev->next = w->next;
      w->next->prev = w->prev;
      if (mFront == w)
         mFront = w->next;
   }
   --mSize;
   mIsMapDirty = true;
   return w;
} 


void CameraSpline::removeAll()
{
   while(front())
      delete remove(front());
  mSize = 0;
}


//-----------------------------------------------------------------------------

void CameraSpline::buildTimeMap()
{
   if (!mIsMapDirty)
      return;

   mTimeMap.clear();
   mTimeMap.reserve(size()*3);      // preallocate
   
   // Initial node and knot value..
   TimeMap map;
   map.mTime = 0;
   map.mDistance = 0;
   mTimeMap.push_back(map);

   Knot ka,kj,ki;
   value(0, &kj, true);
   F32 length = 0.0f;
   ka = kj;

   // Loop through the knots and add nodes. Nodes are added for every knot and
   // whenever the spline length and segment length deviate by epsilon.      
   F32 epsilon = Con::getFloatVariable("CameraSpline::epsilon", 0.90f);
   const F32 Step = 0.05f;
   F32 lt = 0,time = 0;
   do  {
      if ((time += Step) > (mSize - 1))
         time = mSize - 1;

      value(time, &ki, true);
      length += (ki.mPosition - kj.mPosition).len();
      F32 segment = (ki.mPosition - ka.mPosition).len();

      if ((segment / length) < epsilon || time == (mSize - 1) || mFloor(lt) != mFloor(time)) {
         map.mTime = time;
         map.mDistance = length;
         mTimeMap.push_back(map);
         ka = ki;
      }
      kj = ki;
      lt = time;
   }
   while (time < mSize - 1);
   
   mIsMapDirty = false;
}  

 
//-----------------------------------------------------------------------------

void CameraSpline::renderTimeMap()
{
   buildTimeMap();

   glLineWidth(3);
   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   glBegin(GL_LINE_STRIP);

   MRandomLCG random(1376312589 * (U32)this);
   Knot a;
   S32 cr, cg, cb;
   for(Vector<TimeMap>::iterator itr=mTimeMap.begin(); itr != mTimeMap.end(); itr++)
   {
      value(itr->mTime, &a, true);

      cr = random.randI(0,1);
      cg = random.randI(0,1);
      cb = random.randI(0,1);
      glColor4f(cr, cg, cb, 1);
      glVertex3f(a.mPosition.x, a.mPosition.y, a.mPosition.z);
   }

   glEnd();
}   


//-----------------------------------------------------------------------------

F32 CameraSpline::advanceTime(F32 t, S32 delta_ms)
{
   buildTimeMap();
   Knot k;
   value(t, &k, false);
   F32 dist = getDistance(t) + k.mSpeed * (F32(delta_ms) / 1000.0f);
   return getTime(dist);   
}


F32 CameraSpline::advanceDist(F32 t, F32 meters)
{
   buildTimeMap();
   F32 dist = getDistance(t) + meters;
   return getTime(dist);   
}


F32 CameraSpline::getDistance(F32 t)
{
   if (mSize <= 1)
      return 0;

   // Find the nodes spanning the time   
   Vector<TimeMap>::iterator end = mTimeMap.begin() + 1, start;   
   for (; end < (mTimeMap.end() - 1) && end->mTime < t; end++)  {  }
   start = end - 1;

   // Interpolate between the two nodes
   F32 i = (t - start->mTime) / (end->mTime - start->mTime); 
   return start->mDistance + (end->mDistance - start->mDistance) * i;
}   


F32 CameraSpline::getTime(F32 d)
{
   if (mSize <= 1)
      return 0;
   
   // Find nodes spanning the distance
   Vector<TimeMap>::iterator end = mTimeMap.begin() + 1, start;   
   for (; end < (mTimeMap.end() - 1) && end->mDistance < d; end++) {  }
   start = end - 1;

   // Check for duplicate points..
   F32 seg = end->mDistance - start->mDistance;
   if (!seg)
      return end->mTime;
   
   // Interpolate between the two nodes
   F32 i = (d - start->mDistance) / (end->mDistance - start->mDistance); 
   return start->mTime + (end->mTime - start->mTime) * i;
}


//-----------------------------------------------------------------------------

void CameraSpline::value(F32 t, CameraSpline::Knot *result, bool skip_rotation)
{
   // Verify that t is in range [0 >= t > size]
   AssertFatal(t >= 0 && t < size(), "t out of range");
   Knot *p1 = getKnot(mFloor(t));
   Knot *p2 = next(p1);

   F32 i = t - mFloor(t);  // adjust t to 0 to 1 on p1-p2 interval
   if (p1->mPath == Knot::SPLINE)
   {
      Knot *p0 = (p1->mType == Knot::KINK) ? p1 : prev(p1);
      Knot *p3 = (p2->mType == Knot::KINK) ? p2 : next(p2);
      result->mPosition.x = mCatmullrom(i, p0->mPosition.x, p1->mPosition.x, p2->mPosition.x, p3->mPosition.x);
      result->mPosition.y = mCatmullrom(i, p0->mPosition.y, p1->mPosition.y, p2->mPosition.y, p3->mPosition.y);
      result->mPosition.z = mCatmullrom(i, p0->mPosition.z, p1->mPosition.z, p2->mPosition.z, p3->mPosition.z);      
   }
   else
   {   // Linear
      result->mPosition.interpolate(p1->mPosition, p2->mPosition, i);
   }

   if (skip_rotation)
      return;

   buildTimeMap();

   // find the two knots to interpolate rotation and velocity through since some
   // knots are only positional
   S32 start = mFloor(t);
   S32 end   = (p2 == p1) ? start : (start + 1);   
   while (p1->mType == Knot::POSITION_ONLY && p1 != front())
   {
      p1 = prev(p1);
      start--;
   }

   while (p2->mType == Knot::POSITION_ONLY && p2 != back())
   {
      p2 = next(p2);
      end++;
   }

   if (start == end) {
      result->mRotation = p1->mRotation;
      result->mSpeed = p1->mSpeed;
   }
   else {
      F32 c = getDistance(t);
      F32 d1 = getDistance(start);
      F32 d2 = getDistance(end);
      if (d1 == d2) {
         result->mRotation = p2->mRotation;
         result->mSpeed = p2->mSpeed;
      }
      else {
         i  = (c-d1)/(d2-d1);
         result->mRotation.interpolate(p1->mRotation, p2->mRotation, i);
         result->mSpeed = (p1->mSpeed * (1.0f-i)) + (p2->mSpeed * i);
      }
   }
}



