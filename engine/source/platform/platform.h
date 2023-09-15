//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#ifndef _TORQUE_TYPES_H_
#  include "platform/types.h"
#endif

#ifndef _PLATFORMASSERT_H_
#  include "platform/platformAssert.h"
#endif

//------------------------------------------------------------------------------
// Endian conversions
#ifdef TORQUE_LITTLE_ENDIAN

inline U16 convertHostToLEndian(U16 i) { return i; }
inline U16 convertLEndianToHost(U16 i) { return i; }
inline U32 convertHostToLEndian(U32 i) { return i; }
inline U32 convertLEndianToHost(U32 i) { return i; }
inline S16 convertHostToLEndian(S16 i) { return i; }
inline S16 convertLEndianToHost(S16 i) { return i; }
inline S32 convertHostToLEndian(S32 i) { return i; }
inline S32 convertLEndianToHost(S32 i) { return i; }

inline F32 convertHostToLEndian(F32 i) { return i; }
inline F32 convertLEndianToHost(F32 i) { return i; }

inline U16 convertHostToBEndian(U16 i)
{
   return U16((i << 8) | (i >> 8));
}

inline U16 convertBEndianToHost(U16 i)
{
   return U16((i << 8) | (i >> 8));
}

inline S16 convertHostToBEndian(S16 i)
{
   return S16(convertHostToBEndian(U16(i)));
}

inline S16 convertBEndianToHost(S16 i)
{
   return S16(convertBEndianToHost(S16(i)));
}

inline U32 convertHostToBEndian(U32 i)
{
   return ((i << 24) & 0xff000000) |
          ((i <<  8) & 0x00ff0000) |
          ((i >>  8) & 0x0000ff00) |
          ((i >> 24) & 0x000000ff);
}

inline U32 convertBEndianToHost(U32 i)
{
   return ((i << 24) & 0xff000000) |
          ((i <<  8) & 0x00ff0000) |
          ((i >>  8) & 0x0000ff00) |
          ((i >> 24) & 0x000000ff);
}

inline S32 convertHostToBEndian(S32 i)
{
   return S32(convertHostToBEndian(U32(i)));
}

inline S32 convertBEndianToHost(S32 i)
{
   return S32(convertBEndianToHost(S32(i)));
}


#elif defined(TORQUE_BIG_ENDIAN)

inline U16 convertHostToBEndian(U16 i) { return i; }
inline U16 convertBEndianToHost(U16 i) { return i; }
inline U32 convertHostToBEndian(U32 i) { return i; }
inline U32 convertBEndianToHost(U32 i) { return i; }
inline S16 convertHostToBEndian(S16 i) { return i; }
inline S16 convertBEndianToHost(S16 i) { return i; }
inline S32 convertHostToBEndian(S32 i) { return i; }
inline S32 convertBEndianToHost(S32 i) { return i; }

inline U16 convertHostToLEndian(U16 i)
{
   return (i << 8) | (i >> 8);
}
inline U16 convertLEndianToHost(U16 i)
{
   return (i << 8) | (i >> 8);
}
inline U32 convertHostToLEndian(U32 i)
{
   return ((i << 24) & 0xff000000) |
          ((i <<  8) & 0x00ff0000) |
          ((i >>  8) & 0x0000ff00) |
          ((i >> 24) & 0x000000ff);
}
inline U32 convertLEndianToHost(U32 i)
{
   return ((i << 24) & 0xff000000) |
          ((i <<  8) & 0x00ff0000) |
          ((i >>  8) & 0x0000ff00) |
          ((i >> 24) & 0x000000ff);
}


inline F32 convertHostToLEndian(F32 i) 
{ 
   U32 result = convertHostToLEndian( *reinterpret_cast<U32*>(&i) );
   return *reinterpret_cast<F32*>(&result); 
}

inline F32 convertLEndianToHost(F32 i) 
{ 
   U32 result = convertLEndianToHost( *reinterpret_cast<U32*>(&i) );
   return *reinterpret_cast<F32*>(&result); 
}




inline S16 convertHostToLEndian(S16 i) { return S16(convertHostToLEndian(U16(i))); }
inline S16 convertLEndianToHost(S16 i) { return S16(convertLEndianToHost(U16(i))); }
inline S32 convertHostToLEndian(S32 i) { return S32(convertHostToLEndian(U32(i))); }
inline S32 convertLEndianToHost(S32 i) { return S32(convertLEndianToHost(U32(i))); }

#else
#error "Endian define not set"
#endif


//------------------------------------------------------------------------------
// Input structures and functions - all input is pushed into the input event queue
template <class T> class Vector;
class Point2I;


// Theese emuns must be globally scoped so that they work 
// with the inline assembly
enum ProcessorType
{  // x86
   CPU_X86Compatible,
   CPU_Intel_Unknown,
   CPU_Intel_486,
   CPU_Intel_Pentium,
   CPU_Intel_PentiumMMX,
   CPU_Intel_PentiumPro,
   CPU_Intel_PentiumII,
   CPU_Intel_PentiumCeleron,
   CPU_Intel_PentiumIII,
   CPU_Intel_Pentium4,
   CPU_AMD_K6,
   CPU_AMD_K6_2,
   CPU_AMD_K6_3,
   CPU_AMD_Athlon,
   CPU_AMD_Unknown,
   CPU_Cyrix_6x86,
   CPU_Cyrix_MediaGX,
   CPU_Cyrix_6x86MX,
   CPU_Cyrix_GXm,        // Media GX w/ MMX
   CPU_Cyrix_Unknown,
   // PowerPC
   CPU_PowerPC_Unknown,
   CPU_PowerPC_601,
   CPU_PowerPC_603e,
   CPU_PowerPC_603,
   CPU_PowerPC_604e,
   CPU_PowerPC_604,
   CPU_PowerPC_G3,
   CPU_PowerPC_G4,
   CPU_PowerPC_G4u // PPC7450 ultra g4
};

enum x86Properties
{  // x86 properties   
   CPU_PROP_C         = (1<<0),                 
   CPU_PROP_FPU       = (1<<1),      
   CPU_PROP_MMX       = (1<<2),     // Integer-SIMD
   CPU_PROP_3DNOW     = (1<<3),     // AMD Float-SIMD
   CPU_PROP_SSE       = (1<<4),     // PentiumIII SIMD
   CPU_PROP_RDTSC     = (1<<5)     // Read Time Stamp Counter
//   CPU_PROP_SSE2      = (1<<6),   // Pentium4 SIMD
//   CPU_PROP_MP        = (1<<7)      // Multi-processor system
};

enum PPCProperties
{  // PowerPC properties
   CPU_PROP_PPCMIN    = (1<<0),                 
   CPU_PROP_ALTIVEC   = (1<<1),     // Float-SIMD
   CPU_PROP_PPCMP     = (1<<7)      // Multi-processor system
};

struct Platform
{
   static void init();
   static void shutdown();
   static void process();
   static bool doCDCheck();
   static void initWindow(const Point2I &initialSize, const char *name);
   static void enableKeyboardTranslation(void);
   static void disableKeyboardTranslation(void);
   static void setWindowLocked(bool locked);
   static void minimizeWindow();
   static bool excludeOtherInstances(const char *string);
   
   static const Point2I &getWindowSize();
   static void setWindowSize( U32 newWidth, U32 newHeight );
   static float getRandom();
   static void AlertOK(const char *windowTitle, const char *message);
   static bool AlertOKCancel(const char *windowTitle, const char *message);
   static bool AlertRetry(const char *windowTitle, const char *message);
   
   struct LocalTime {
      U8  sec;        // seconds after minute (0-59)
      U8  min;        // Minutes after hour (0-59)
      U8  hour;       // Hours after midnight (0-23)
      U8  month;      // Month (0-11; 0=january)
      U8  monthday;   // Day of the month (1-31)
      U8  weekday;    // Day of the week (0-6, 6=sunday)
      U16 year;       // current year minus 1900
      U16 yearday;    // Day of year (0-365)
      bool isdst;     // true if daylight savings time is active
   };
   
   static void getLocalTime(LocalTime &);

   struct FileInfo {
      const char* pFullPath;
      const char* pFileName;
      U32 fileSize;
   };
   static bool cdFileExists(const char *filePath, const char *volumeName, S32 serialNum);
   static void fileToLocalTime(const FileTime &ft, LocalTime *lt);
   // compare file times returns < 0 if a is earlier than b, >0 if b is earlier than a
   static S32 compareFileTimes(const FileTime &a, const FileTime &b);
   // Process control
  public:
   static void postQuitMessage(const U32 in_quitVal);
   static void debugBreak();
   static void forceShutdown(S32 returnValue);

   static U32  getTime();
   static U32  getVirtualMilliseconds();
   static U32  getRealMilliseconds();
   static void advanceTime(U32 delta);

   // Directory functions.  Dump path returns false iff the directory cannot be
   //  opened.
  public:
   static StringTableEntry getWorkingDirectory();
   static bool dumpPath(const char *in_pBasePath, Vector<FileInfo>& out_rFileVector);
   static bool getFileTimes(const char *filePath, FileTime *createTime, FileTime *modifyTime);
   static bool isFile(const char *pFilePath);
   static bool isDirectory(const char *pDirPath);
   static bool isSubDirectory(const char *pParent, const char *pDir);
   

   static bool createPath(const char *path); // create a directory path
   static struct SystemInfo_struct
   {
      struct Processor
      {
         ProcessorType type;
         const char *name;
         U32         mhz;
         U32         properties;      // CPU type specific enum
      }processor;
   }SystemInfo;

   // Web page launch function:
   static bool openWebBrowser( const char* webAddress );

   static const char* getLoginPassword();
   static bool setLoginPassword( const char* password );

   static const char* getClipboard();
   static bool setClipboard(const char *text);
};


struct Processor
{
   static void init();
};


//------------------------------------------------------------------------------
// time manager generates a ServerTimeEvent / ClientTimeEvent, FrameEvent combo
// every other time its process is called.
struct TimeManager
{
   static void process();
};

// the entry point of the app is in the platform code...
// it calls out into game code at GameMain

// all input goes through the game input event queue
// whether or not it is used (replaying a journal, etc)
// is up to the game code.  The game must copy the event data out.

struct Event;

//------------------------------------------------------------------------------
// String functions

extern char*       dStrcat(char *dst, const char *src);
extern char*       dStrncat(char* dst, const char* src, dsize_t len);
extern char*       dStrcatl(char *dst, dsize_t dstSize, ...);
extern int         dStrcmp(const char *str1, const char *str2);
extern int         dStricmp(const char *str1, const char *str2);
extern int         dStrncmp(const char *str1, const char *str2, dsize_t len);
extern int         dStrnicmp(const char *str1, const char *str2, dsize_t len);
extern char*       dStrcpy(char *dst, const char *src);
extern char*       dStrcpyl(char *dst, dsize_t dstSize, ...);
extern char*       dStrncpy(char *dst, const char *src, dsize_t len);
extern dsize_t      dStrlen(const char *str);
extern char*       dStrupr(char *str);
extern char*       dStrlwr(char *str);
extern char*       dStrchr(char *str, int c);
extern const char* dStrchr(const char *str, int c);
extern char*       dStrrchr(char *str, int c);
extern const char* dStrrchr(const char *str, int c);
extern dsize_t      dStrspn(const char *str, const char *set);
extern dsize_t      dStrcspn(const char *str, const char *set);

extern char*       dStrstr(const char *str1, const char *str2);

extern char*       dStrtok(char *str, const char *sep);

extern int         dAtoi(const char *str);
extern float       dAtof(const char *str);
extern bool        dAtob(const char *str);

extern void   dPrintf(const char *format, ...);
extern int    dVprintf(const char *format, void *arglist);
extern int    dSprintf(char *buffer, dsize_t bufferSize, const char *format, ...);
extern int    dVsprintf(char *buffer, dsize_t bufferSize, const char *format, void *arglist);
extern int    dSscanf(const char *buffer, const char *format, ...);
extern int    dFflushStdout();
extern int    dFflushStderr();
 
inline char dToupper(const char c) { if (c >= char('a') && c <= char('z')) return char(c + 'A' - 'a'); else return c; }
inline char dTolower(const char c) { if (c >= char('A') && c <= char('Z')) return char(c - 'A' + 'a'); else return c; }

extern bool dIsalnum(const char c);
extern bool dIsalpha(const char c);
extern bool dIsdigit(const char c);
extern bool dIsspace(const char c);

//------------------------------------------------------------------------------
// Misc StdLib functions


#define QSORT_CALLBACK FN_CDECL
void dQsort(void *base, U32 nelem, U32 width, int (QSORT_CALLBACK *fcmp)(const void *, const void *));


//------------------------------------------------------------------------------
// ConsoleObject GetClassNameFn
class ConsoleObject;
const char* __dyncreate_getNameFromType(ConsoleObject *);


//------------------------------------------------------------------------------
// Memory functions

namespace Memory {
   dsize_t getMemoryUsed();
   dsize_t getMemoryAllocated();
} // namespace Memory

extern void* FN_CDECL operator new(dsize_t size, void* ptr);

template <class T>
inline T* constructInPlace(T* p)
{
   return new(p) T;
}

template <class T>
inline void destructInPlace(T* p)
{
   p->~T();
}

#if !defined(TORQUE_DISABLE_MEMORY_MANAGER)
   extern void* FN_CDECL operator new(dsize_t size, const char*, const U32);
   extern void* FN_CDECL operator new[](dsize_t size, const char*, const U32);
   extern void  FN_CDECL operator delete(void* ptr);
   extern void  FN_CDECL operator delete[](void* ptr);
   #define new new(__FILE__, __LINE__)
#endif 

#define placenew(x) new(x)
#define dMalloc(x) dMalloc_r(x, __FILE__, __LINE__)
#define dStrdup(x) dStrdup_r(x, __FILE__, __LINE__)

extern char* dStrdup_r(const char *src, const char*, dsize_t);

extern void  setBreakAlloc(dsize_t);
extern void  setMinimumAllocUnit(dsize_t);
extern void* dMalloc_r(dsize_t in_size, const char*, const dsize_t);
extern void  dFree(void* in_pFree);
extern void* dRealloc(void* in_pResize, dsize_t in_size);
extern void* dRealMalloc(dsize_t);
extern void  dRealFree(void*);

extern void* dMemcpy(void *dst, const void *src, dsize_t size);
extern void* dMemmove(void *dst, const void *src, dsize_t size);
extern void* dMemset(void *dst, int c, dsize_t size);
extern int   dMemcmp(const void *ptr1, const void *ptr2, dsize_t size);

//------------------------------------------------------------------------------
// Graphics functions

class GFont;
extern GFont *createFont(const char *name, dsize_t size);


//------------------------------------------------------------------------------
// FileIO functions
typedef void* FILE_HANDLE;
enum DFILE_STATUS
{
   DFILE_OK = 1
};   

extern bool dFileDelete(const char *name);
extern bool dFileTouch(const char *name);

extern FILE_HANDLE dOpenFileRead(const char *name, DFILE_STATUS &error);
extern FILE_HANDLE dOpenFileReadWrite(const char *name, bool append, DFILE_STATUS &error);
extern int dFileRead(FILE_HANDLE handle, U32 bytes, char *dst, DFILE_STATUS &error);
extern int dFileWrite(FILE_HANDLE handle, U32 bytes, const char *dst, DFILE_STATUS &error);
extern void dFileClose(FILE_HANDLE handle);


//------------------------------------------------------------------------------
// Math
struct Math
{
   static void init(U32 properties = 0);   // 0 == detect available hardware
};


//------------------------------------------------------------------------------
// Networking
struct NetAddress;

typedef S32 NetSocket;
const NetSocket InvalidSocket = -1;

struct Net {
   enum Error {
      NoError,
      WrongProtocolType,
      InvalidPacketProtocol,
      WouldBlock,
      NotASocket,
      UnknownError
   };
   enum Protocol {
      UDPProtocol,
      IPXProtocol,
      TCPProtocol
   };
   static bool init();
   static void shutdown();

   // Unreliable net functions (UDP)
   // sendto is for sending data
   // all incoming data comes in on packetReceiveEventType
   // App can only open one unreliable port... who needs more? ;)
   static bool openPort(S32 connectPort);
   static void closePort();
   static Error sendto(const NetAddress *address, const U8 *buffer, S32 bufferSize);

   // Reliable net functions (TCP)
   // all incoming messages come in on the Connected* events
   static NetSocket openListenPort(U16 port);
   static NetSocket openConnectTo(const char *stringAddress); // does the DNS resolve etc.
   static void closeConnectTo(NetSocket socket);
   static Error sendtoSocket(NetSocket socket, const U8 *buffer, S32 bufferSize);
   
   static void process();

   static bool compareAddresses(const NetAddress *a1, const NetAddress *a2);
   static bool stringToAddress(const char *addressString, NetAddress *address);
   static void addressToString(const NetAddress *address, char addressString[256]);

   // lower level socked based network functions
   static NetSocket openSocket();
   static Error closeSocket(NetSocket socket);

   static Error connect(NetSocket socket, const NetAddress *address);
   static Error listen(NetSocket socket, S32 maxConcurrentListens);
   static NetSocket accept(NetSocket acceptSocket, NetAddress *remoteAddress);
   
   static Error bind(NetSocket socket, U16    port);
   static Error setBufferSize(NetSocket socket, S32 bufferSize);
   static Error setBroadcast(NetSocket socket, bool broadcastEnable);
   static Error setBlocking(NetSocket socket, bool blockingIO);

   static Error send(NetSocket socket, const U8 *buffer, S32 bufferSize);
   static Error recv(NetSocket socket, U8 *buffer, S32 bufferSize, S32 *bytesRead);
};


#endif
