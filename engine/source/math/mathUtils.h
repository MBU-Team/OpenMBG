//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _MATHUTILS_H_
#define _MATHUTILS_H_

class Point3F;
class MatrixF;

/// Miscellaneous math utility functions.
namespace MathUtils
{

    /// Creates orientation matrix from a direction vector.  Assumes ( 0 0 1 ) is up.
    MatrixF createOrientFromDir( Point3F &direction );

    /// Creates random direction given angle parameters similar to the particle system.
    ///
    /// The angles are relative to the specified axis. Both phi and theta are in degrees.
    Point3F randomDir( Point3F &axis, F32 thetaAngleMin, F32 thetaAngleMax, F32 phiAngleMin = 0.0, F32 phiAngleMax = 360.0 );

    /// Returns yaw and pitch angles from a given vector.
    ///
    /// Angles are in RADIANS.
    ///
    /// Assumes north is (0.0, 1.0, 0.0), the degrees move upwards clockwise.
    ///
    /// The range of yaw is 0 - 2PI.  The range of pitch is -PI/2 - PI/2.
    ///
    /// <b>ASSUMES Z AXIS IS UP</b>
    void    getAnglesFromVector( VectorF &vec, F32 &yawAng, F32 &pitchAng );

    /// Returns vector from given yaw and pitch angles.
    ///    
    /// Angles are in RADIANS.
    ///
    /// Assumes north is (0.0, 1.0, 0.0), the degrees move upwards clockwise.
    ///
    /// The range of yaw is 0 - 2PI.  The range of pitch is -PI/2 - PI/2.
    ///
    /// <b>ASSUMES Z AXIS IS UP</b>
    void    getVectorFromAngles( VectorF &vec, F32 &yawAng, F32 &pitchAng );
}

#endif // _MATHUTILS_H_
