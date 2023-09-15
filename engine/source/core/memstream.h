//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _MEMSTREAM_H_
#define _MEMSTREAM_H_

//Includes
#ifndef _STREAM_H_
#include "core/stream.h"
#endif

class MemStream : public Stream {
   typedef Stream Parent;

  protected:
   U32 const cm_bufferSize;
   void*  m_pBufferBase;

   U32 m_instCaps;
   U32 m_currentPosition;

  public:
   MemStream(const U32  in_bufferSize,
             void*      io_pBuffer,
             const bool in_allowRead  = true,
             const bool in_allowWrite = true);
   ~MemStream();

   // Mandatory overrides from Stream
  protected:
   bool _read(const U32 in_numBytes,  void* out_pBuffer);
   bool _write(const U32 in_numBytes, const void* in_pBuffer);
  public:
   bool hasCapability(const Capability) const;
   U32  getPosition() const;
   bool setPosition(const U32 in_newPosition);

   // Mandatory overrides from Stream
  public:
   U32  getStreamSize();
};

#endif //_MEMSTREAM_H_
