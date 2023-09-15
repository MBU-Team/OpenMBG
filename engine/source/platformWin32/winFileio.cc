//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platformWin32/platformWin32.h"
#include "core/fileio.h"
#include "core/tVector.h"
#include "core/stringTable.h"
#include "console/console.h"

// Microsoft VC++ has this POSIX header in the wrong directory
#if defined(TORQUE_COMPILER_VISUALC)
#include <sys/utime.h>
#elif defined (TORQUE_COMPILER_GCC)
#include <time.h>
#include <sys/utime.h>
#else
#include <utime.h>
#endif

//-------------------------------------- Helper Functions
static void forwardslash(char *str)
{
   while(*str)
   {
      if(*str == '\\')
         *str = '/';
      str++;
   }
}

static void backslash(char *str)
{
   while(*str)
   {
      if(*str == '/')
         *str = '\\';
      str++;
   }
}

//-----------------------------------------------------------------------------
bool dFileDelete(const char * name)
{
   if(!name || (dStrlen(name) >= MAX_PATH))
      return(false);
   return(::DeleteFileA(name));
}

bool dFileTouch(const char * name)
{
   // change the modified time to the current time (0byte WriteFile fails!)
   return(utime(name, 0) != -1);
}

//-----------------------------------------------------------------------------
// Constructors & Destructor
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// After construction, the currentStatus will be Closed and the capabilities
// will be 0.
//-----------------------------------------------------------------------------
File::File()
: currentStatus(Closed), capability(0)
{
    AssertFatal(sizeof(HANDLE) == sizeof(void *), "File::File: cannot cast void* to HANDLE");

    handle = (void *)INVALID_HANDLE_VALUE;
}

//-----------------------------------------------------------------------------
// insert a copy constructor here... (currently disabled)
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
File::~File()
{
    close();
    handle = (void *)INVALID_HANDLE_VALUE;
}


//-----------------------------------------------------------------------------
// Open a file in the mode specified by openMode (Read, Write, or ReadWrite).
// Truncate the file if the mode is either Write or ReadWrite and truncate is
// true.
//
// Sets capability appropriate to the openMode.
// Returns the currentStatus of the file.
//-----------------------------------------------------------------------------
File::Status File::open(const char *filename, const AccessMode openMode)
{
   static char filebuf[2048];
   dStrcpy(filebuf, filename);
   backslash(filebuf);
   filename = filebuf;
   
    AssertFatal(NULL != filename, "File::open: NULL filename");
    AssertWarn(INVALID_HANDLE_VALUE == (HANDLE)handle, "File::open: handle already valid");

    // Close the file if it was already open...
    if (Closed != currentStatus)
        close();

    // create the appropriate type of file...
    switch (openMode)
    {
    case Read:
        handle = (void *)CreateFileA(filename,
                                    GENERIC_READ,
                                    FILE_SHARE_READ,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                    NULL);
        break;
    case Write:
        handle = (void *)CreateFileA(filename,
                                    GENERIC_WRITE,
                                    0,
                                    NULL,
                                    CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                    NULL);
        break;
    case ReadWrite:
        handle = (void *)CreateFileA(filename,
                                    GENERIC_WRITE | GENERIC_READ,
                                    0,
                                    NULL,
                                    OPEN_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                    NULL);
        break;
    case WriteAppend:
        handle = (void *)CreateFileA(filename,
                                    GENERIC_WRITE,
                                    0,
                                    NULL,
                                    OPEN_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                    NULL);
        break;

    default:
        AssertFatal(false, "File::open: bad access mode");    // impossible
      }
    
    if (INVALID_HANDLE_VALUE == (HANDLE)handle)                // handle not created successfully
        return setStatus();
    else
    {
        // successfully created file, so set the file capabilities...
        switch (openMode)
        {
        case Read:
            capability = U32(FileRead);
            break;
        case Write:
        case WriteAppend:
            capability = U32(FileWrite);
            break;
        case ReadWrite:
            capability = U32(FileRead)  |
                         U32(FileWrite);
            break;
        default:
            AssertFatal(false, "File::open: bad access mode");
        }
        return currentStatus = Ok;                                // success!
    }
}

//-----------------------------------------------------------------------------
// Get the current position of the file pointer.
//-----------------------------------------------------------------------------
U32 File::getPosition() const
{
    AssertFatal(Closed != currentStatus, "File::getPosition: file closed");
    AssertFatal(INVALID_HANDLE_VALUE != (HANDLE)handle, "File::getPosition: invalid file handle");

    return SetFilePointer((HANDLE)handle,
                          0,                                    // how far to move
                          NULL,                                    // pointer to high word
                          FILE_CURRENT);                        // from what point
}

//-----------------------------------------------------------------------------
// Set the position of the file pointer.
// Absolute and relative positioning is supported via the absolutePos
// parameter.
//
// If positioning absolutely, position MUST be positive - an IOError results if
// position is negative.
// Position can be negative if positioning relatively, however positioning
// before the start of the file is an IOError.
//
// Returns the currentStatus of the file.
//-----------------------------------------------------------------------------
File::Status File::setPosition(S32 position, bool absolutePos)
{
    AssertFatal(Closed != currentStatus, "File::setPosition: file closed");
    AssertFatal(INVALID_HANDLE_VALUE != (HANDLE)handle, "File::setPosition: invalid file handle");
    
    if (Ok != currentStatus && EOS != currentStatus)
        return currentStatus;
    
    U32 finalPos;
    switch (absolutePos)
    {
    case true:                                                    // absolute position
        AssertFatal(0 <= position, "File::setPosition: negative absolute position");
        
        // position beyond EOS is OK
        finalPos = SetFilePointer((HANDLE)handle,
                                  position,
                                  NULL,
                                  FILE_BEGIN);
        break;
    case false:                                                    // relative position
        AssertFatal((getPosition() >= (U32)abs(position) && 0 > position) || 0 <= position, "File::setPosition: negative relative position");
        
        // position beyond EOS is OK
        finalPos = SetFilePointer((HANDLE)handle,
                                  position,
                                  NULL,
                                  FILE_CURRENT);
    }

    if (0xffffffff == finalPos)
        return setStatus();                                        // unsuccessful
    else if (finalPos >= getSize())
        return currentStatus = EOS;                                // success, at end of file
    else
        return currentStatus = Ok;                                // success!
}

//-----------------------------------------------------------------------------
// Get the size of the file in bytes.
// It is an error to query the file size for a Closed file, or for one with an
// error status.
//-----------------------------------------------------------------------------
U32 File::getSize() const
{
    AssertWarn(Closed != currentStatus, "File::getSize: file closed");
    AssertFatal(INVALID_HANDLE_VALUE != (HANDLE)handle, "File::getSize: invalid file handle");
    
    if (Ok == currentStatus || EOS == currentStatus)
    {
        DWORD high;
        return GetFileSize((HANDLE)handle, &high);                // success!
    }
    else
        return 0;                                                // unsuccessful
}

//-----------------------------------------------------------------------------
// Flush the file.
// It is an error to flush a read-only file.
// Returns the currentStatus of the file.
//-----------------------------------------------------------------------------
File::Status File::flush()
{
    AssertFatal(Closed != currentStatus, "File::flush: file closed");
    AssertFatal(INVALID_HANDLE_VALUE != (HANDLE)handle, "File::flush: invalid file handle");
    AssertFatal(true == hasCapability(FileWrite), "File::flush: cannot flush a read-only file");

    if (0 != FlushFileBuffers((HANDLE)handle))
        return setStatus();                                        // unsuccessful
    else
        return currentStatus = Ok;                                // success!
}

//-----------------------------------------------------------------------------
// Close the File.
//
// Returns the currentStatus
//-----------------------------------------------------------------------------
File::Status File::close()
{
    // check if it's already closed...
    if (Closed == currentStatus)
        return currentStatus;

    // it's not, so close it...
    if (INVALID_HANDLE_VALUE != (HANDLE)handle)
    {
        if (0 == CloseHandle((HANDLE)handle))
            return setStatus();                                    // unsuccessful
    }
    handle = (void *)INVALID_HANDLE_VALUE;
    return currentStatus = Closed;
}

//-----------------------------------------------------------------------------
// Self-explanatory.
//-----------------------------------------------------------------------------
File::Status File::getStatus() const
{
    return currentStatus;
}

//-----------------------------------------------------------------------------
// Sets and returns the currentStatus when an error has been encountered.
//-----------------------------------------------------------------------------
File::Status File::setStatus()
{
    switch (GetLastError())
    {
    case ERROR_INVALID_HANDLE:
    case ERROR_INVALID_ACCESS:
    case ERROR_TOO_MANY_OPEN_FILES:
    case ERROR_FILE_NOT_FOUND:
    case ERROR_SHARING_VIOLATION:
    case ERROR_HANDLE_DISK_FULL:
          return currentStatus = IOError;

    default:
          return currentStatus = UnknownError;
    }
}

//-----------------------------------------------------------------------------
// Sets and returns the currentStatus to status.
//-----------------------------------------------------------------------------
File::Status File::setStatus(File::Status status)
{
    return currentStatus = status;
}

//-----------------------------------------------------------------------------
// Read from a file.
// The number of bytes to read is passed in size, the data is returned in src.
// The number of bytes read is available in bytesRead if a non-Null pointer is
// provided.
//-----------------------------------------------------------------------------
File::Status File::read(U32 size, char *dst, U32 *bytesRead)
{
    AssertFatal(Closed != currentStatus, "File::read: file closed");
    AssertFatal(INVALID_HANDLE_VALUE != (HANDLE)handle, "File::read: invalid file handle");
    AssertFatal(NULL != dst, "File::read: NULL destination pointer");
    AssertFatal(true == hasCapability(FileRead), "File::read: file lacks capability");
    AssertWarn(0 != size, "File::read: size of zero");
    
    if (Ok != currentStatus || 0 == size)
        return currentStatus;
    else
    {
        DWORD lastBytes;
        DWORD *bytes = (NULL == bytesRead) ? &lastBytes : (DWORD *)bytesRead;
        if (0 != ReadFile((HANDLE)handle, dst, size, bytes, NULL))
        {
            if(*((U32 *)bytes) != size)
                return currentStatus = EOS;                        // end of stream
        }
        else
            return setStatus();                                    // unsuccessful
    }
    return currentStatus = Ok;                                    // successfully read size bytes
}

//-----------------------------------------------------------------------------
// Write to a file.
// The number of bytes to write is passed in size, the data is passed in src.
// The number of bytes written is available in bytesWritten if a non-Null
// pointer is provided.
//-----------------------------------------------------------------------------
File::Status File::write(U32 size, const char *src, U32 *bytesWritten)
{
    AssertFatal(Closed != currentStatus, "File::write: file closed");
    AssertFatal(INVALID_HANDLE_VALUE != (HANDLE)handle, "File::write: invalid file handle");
    AssertFatal(NULL != src, "File::write: NULL source pointer");
    AssertFatal(true == hasCapability(FileWrite), "File::write: file lacks capability");
    AssertWarn(0 != size, "File::write: size of zero");
    
    if ((Ok != currentStatus && EOS != currentStatus) || 0 == size)
        return currentStatus;
    else
    {
        DWORD lastBytes;
        DWORD *bytes = (NULL == bytesWritten) ? &lastBytes : (DWORD *)bytesWritten;
        if (0 != WriteFile((HANDLE)handle, src, size, bytes, NULL))
            return currentStatus = Ok;                            // success!
        else
            return setStatus();                                    // unsuccessful
    }
}

//-----------------------------------------------------------------------------
// Self-explanatory.
//-----------------------------------------------------------------------------
bool File::hasCapability(Capability cap) const
{
    return (0 != (U32(cap) & capability));
}

S32 Platform::compareFileTimes(const FileTime &a, const FileTime &b)
{
   if(a.v2 > b.v2)
      return 1;
   if(a.v2 < b.v2)
      return -1;
   if(a.v1 > b.v1)
      return 1;
   if(a.v1 < b.v1)
      return -1;
   return 0;
}

static bool recurseDumpPath(const char *path, const char *pattern, Vector<Platform::FileInfo> &fileVector)
{
   char search[1024];
   WIN32_FIND_DATAA findData;

   dSprintf(search, sizeof(search), "%s/%s", path, pattern);
   
   HANDLE handle = FindFirstFileA(search, &findData);
   if (handle == INVALID_HANDLE_VALUE)
      return false;

   do
   {
      if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
         // make sure it is a directory
         if (findData.dwFileAttributes & (FILE_ATTRIBUTE_OFFLINE|FILE_ATTRIBUTE_SYSTEM) )                             
            continue;

         // skip . and .. directories
         if (dStrcmp(findData.cFileName, ".") == 0 || dStrcmp(findData.cFileName, "..") == 0)
            continue;

         char child[1024];
         dSprintf(child, sizeof(child), "%s/%s", path, findData.cFileName);
         recurseDumpPath(child, pattern, fileVector);
      }      
      else
      {
         // make sure it is the kind of file we're looking for
         if (findData.dwFileAttributes & 
             (FILE_ATTRIBUTE_DIRECTORY|                                      
              FILE_ATTRIBUTE_OFFLINE|
              FILE_ATTRIBUTE_SYSTEM|
              FILE_ATTRIBUTE_TEMPORARY) )                             
            continue;
         
         // add it to the list
         fileVector.increment();
         Platform::FileInfo& rInfo = fileVector.last();

         rInfo.pFullPath = StringTable->insert(path);
         rInfo.pFileName = StringTable->insert(findData.cFileName);
         rInfo.fileSize  = findData.nFileSizeLow;
      }

   }while(FindNextFileA(handle, &findData));

   FindClose(handle);
   return true;
}   


//--------------------------------------

bool Platform::getFileTimes(const char *filePath, FileTime *createTime, FileTime *modifyTime)
{
   WIN32_FIND_DATAA findData;
   HANDLE h = FindFirstFileA(filePath, &findData);
   if(h == INVALID_HANDLE_VALUE)
      return false;

   if(createTime)
   {   
      createTime->v1 = findData.ftCreationTime.dwLowDateTime;
      createTime->v2 = findData.ftCreationTime.dwHighDateTime;
   }
   if(modifyTime)
   {
      modifyTime->v1 = findData.ftLastWriteTime.dwLowDateTime;
      modifyTime->v2 = findData.ftLastWriteTime.dwHighDateTime;
   }
   FindClose(h);
   return true;
}

//--------------------------------------
bool Platform::createPath(const char *file)
{
   char pathbuf[1024];
   const char *dir;
   pathbuf[0] = 0;
   U32 pathLen = 0;
   
   while((dir = dStrchr(file, '/')) != NULL)
   {
      dStrncpy(pathbuf + pathLen, file, dir - file);
      pathbuf[pathLen + dir-file] = 0;
      bool ret = CreateDirectoryA(pathbuf, NULL);
      pathLen += dir - file;
      pathbuf[pathLen++] = '/';
      file = dir + 1;
   }
   return true;
}

bool Platform::cdFileExists(const char *filePath, const char *volumeName, S32 serialNum)
{
   if (!filePath || !filePath[0])
      return true;

   //first find the CD device...
   char fileBuf[256];
   char drivesBuf[256];
   S32 length = GetLogicalDriveStringsA(256, drivesBuf);
   char *drivePtr = drivesBuf;
   while (S32(drivePtr - drivesBuf) < length)
   {
      char driveVolume[256], driveFileSystem[256];
      U32 driveSerial, driveFNLength, driveFlags;
      if ((dStricmp(drivePtr, "A:\\") != 0 && dStricmp(drivePtr, "B:\\") != 0) &&
          GetVolumeInformationA((const char*)drivePtr, &driveVolume[0], (unsigned long)255,
                               (unsigned long*)&driveSerial, (unsigned long*)&driveFNLength,
                               (unsigned long*)&driveFlags, &driveFileSystem[0], (unsigned long)255))
      {
#if defined (TORQUE_DEBUG) || defined (INTERNAL_RELEASE)
         Con::printf("Found Drive: %s, vol: %s, serial: %d", drivePtr, driveVolume, driveSerial);
#endif
         //see if the volume and serial number match
         if (!dStricmp(volumeName, driveVolume) && (!serialNum || (serialNum == driveSerial)))
         {
            //see if the file exists on this volume
            if(dStrlen(drivePtr) == 3 && drivePtr[2] == '\\' && filePath[0] == '\\')
               dSprintf(fileBuf, sizeof(fileBuf), "%s%s", drivePtr, filePath + 1);
            else
               dSprintf(fileBuf, sizeof(fileBuf), "%s%s", drivePtr, filePath);
#if defined (TORQUE_DEBUG) || defined (INTERNAL_RELEASE)
            Con::printf("Looking for file: %s on %s", fileBuf, driveVolume);
#endif
            WIN32_FIND_DATAA findData;
            HANDLE h = FindFirstFileA(fileBuf, &findData);
            if(h != INVALID_HANDLE_VALUE)
            {
               FindClose(h);
               return true;
            }
            FindClose(h);
         }
      }

      //check the next drive
      drivePtr += dStrlen(drivePtr) + 1;
   }

   return false;
}

//--------------------------------------
bool Platform::dumpPath(const char *path, Vector<Platform::FileInfo> &fileVector)
{
   return recurseDumpPath(path, "*", fileVector);
}


//--------------------------------------
StringTableEntry Platform::getWorkingDirectory()
{
   static StringTableEntry cwd = NULL;
   if (!cwd)
   {
      char cwd_buf[2048];
      GetCurrentDirectoryA(2047, cwd_buf);
      forwardslash(cwd_buf);
      cwd = StringTable->insert(cwd_buf);
   }
   return cwd;
}



//--------------------------------------
bool Platform::isFile(const char *pFilePath)
{
   if (!pFilePath || !*pFilePath) 
      return false;

   // Get file info 
   WIN32_FIND_DATAA findData;
   HANDLE handle = FindFirstFileA(pFilePath, &findData);
   FindClose(handle);

   if(handle == INVALID_HANDLE_VALUE)
      return false;

   // if the file is a Directory, Offline, System or Temporary then FALSE
   if (findData.dwFileAttributes & 
       (FILE_ATTRIBUTE_DIRECTORY|                                      
        FILE_ATTRIBUTE_OFFLINE|
        FILE_ATTRIBUTE_SYSTEM|
        FILE_ATTRIBUTE_TEMPORARY) )                             
      return false;

   // must be a real file then
   return true;
}


//--------------------------------------
bool Platform::isDirectory(const char *pDirPath)
{
   if (!pDirPath || !*pDirPath) 
      return false;

   // Get file info 
   WIN32_FIND_DATAA findData;
   HANDLE handle = FindFirstFileA(pDirPath, &findData);
   FindClose(handle);

   if(handle == INVALID_HANDLE_VALUE)
      return false;
   
   // if the file is a Directory, Offline, System or Temporary then FALSE
   if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
   {
      // make sure it's a valid game directory
      if (findData.dwFileAttributes & (FILE_ATTRIBUTE_OFFLINE|FILE_ATTRIBUTE_SYSTEM) )                             
         return false;

      // must be a directory
      return true;      
   }

   return false;
}



//--------------------------------------
bool Platform::isSubDirectory(const char *pParent, const char *pDir)
{
   if (!pParent || !*pDir) 
      return false;

   // this is somewhat of a brute force method but we need to be 100% sure
   // that the user cannot enter things like ../dir or /dir etc,...
   WIN32_FIND_DATAA findData;
   HANDLE handle = FindFirstFileA(avar("%s/*", pParent), &findData);
   if (handle == INVALID_HANDLE_VALUE)
      return false;
   do
   {
      // if it is a directory...
      if (findData.dwFileAttributes & 
          (FILE_ATTRIBUTE_DIRECTORY|                                      
           FILE_ATTRIBUTE_OFFLINE|
           FILE_ATTRIBUTE_SYSTEM|
           FILE_ATTRIBUTE_TEMPORARY) )                             
      {
         // and the names match
         if (dStrcmp(pDir, findData.cFileName ) == 0)
         {
            // then we have a real sub directory
            FindClose(handle);
            return true;
         }
      }
   }while(FindNextFileA(handle, &findData));

   FindClose(handle);
   return false;
}



