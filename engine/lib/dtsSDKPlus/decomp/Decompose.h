//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

/**** Decompose.h - Basic declarations ****/
#ifndef _H_Decompose
#define _H_Decompose

namespace GraphicGems
{

   typedef struct {double x, y, z, w;} Quat; /* Quaternion */
   enum QuatPart {X, Y, Z, W};
   typedef Quat HVect; /* Homogeneous 3D vector */
   typedef double HMatrix[4][4]; /* Right-handed, for column vectors */
   typedef struct {
       HVect t;	/* Translation components */
       Quat  q;	/* Essential rotation	  */
       Quat  u;	/* Stretch rotation	  */
       HVect k;	/* Stretch factors	  */
       double f;	/* Sign of determinant	  */
   } AffineParts;
   double polar_decomp(HMatrix M, HMatrix Q, HMatrix S);
   HVect spect_decomp(HMatrix S, HMatrix U);
   Quat snuggle(Quat q, HVect *k);
   void decomp_affine(HMatrix A, AffineParts *parts);
   void invert_affine(AffineParts *parts, AffineParts *inverse);

}; // namespace GraphicGems

#endif
