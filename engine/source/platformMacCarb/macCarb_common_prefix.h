//------------------------------
//maccarb_common_prefix.h
//------------------------------

#define TORQUE_OS_MAC_CARB       1     // always defined right now...

#if defined(TORQUE_OS_MAC_OSX)
#define Z_PREFIX  // OSX comes with zlib, so generate unique symbols.
#endif

// defines for the mac headers to activate proper Carbon codepaths.
#define TARGET_API_MAC_CARBON    1   // apple carbon header flag to take right defpaths.
//#define OTCARBONAPPLICATION      1   // means we can use the old-style funcnames

#ifndef CARBON_VERSION
#if defined(TORQUE_OS_MAC_OSX)
#define CARBON_VERSION             0x0120      // what carbon shipped with 10.0???? !!!!TBD
#else
#define CARBON_VERSION             0x0104      // the carbon version # we are targeting.
#endif
#endif

