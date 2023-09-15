//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (c) 2002 GarageGames.Com
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "core/dnet.h"
#include "console/simBase.h"
#include "sim/netConnection.h"
#include "core/bitStream.h"
#include "sim/netObject.h"
#include "core/resManager.h"


class FileDownloadRequestEvent : public NetEvent
{
public:
   enum {
      MaxFileNames = 31,
   };
   U32 nameCount;
   char mFileNames[MaxFileNames][256];
   FileDownloadRequestEvent(Vector<char *> *nameList = NULL)
   {
      nameCount = 0;
      if(nameList)
      {
         nameCount = nameList->size();
         if(nameCount > MaxFileNames)
            nameCount = MaxFileNames;
         for(U32 i = 0; i < nameCount; i++)
         {
            dStrcpy(mFileNames[i], (*nameList)[i]);
            Con::printf("Sending request for file %s", mFileNames[i]);
         }
      }
   }
   virtual void pack(NetConnection *, BitStream *bstream)
   {
      bstream->writeRangedU32(nameCount, 0, MaxFileNames);
      for(U32 i = 0; i < nameCount; i++)
         bstream->writeString(mFileNames[i]);
   }
   virtual void write(NetConnection *, BitStream *bstream)
   {
      bstream->writeRangedU32(nameCount, 0, MaxFileNames);
      for(U32 i = 0; i < nameCount; i++)
         bstream->writeString(mFileNames[i]);
   }
   virtual void unpack(NetConnection *, BitStream *bstream)
   {
      nameCount = bstream->readRangedU32(0, MaxFileNames);
      for(U32 i = 0; i < nameCount; i++)
         bstream->readString(mFileNames[i]);
   }
   virtual void process(NetConnection *connection)
   {
      U32 i;
      for(i = 0; i < nameCount; i++)
         if(connection->startSendingFile(mFileNames[i]))
            break;
      if(i == nameCount)
         connection->startSendingFile(NULL);  // none of the files were sent
   }
   DECLARE_CONOBJECT(FileDownloadRequestEvent);
};
IMPLEMENT_CO_NETEVENT_V1(FileDownloadRequestEvent);

class FileChunkEvent : public NetEvent
{
public:
   enum
   {
      ChunkSize = 63,
   };
   U8 chunkData[ChunkSize];
   U32 chunkLen;
   FileChunkEvent(U8 *data = NULL, U32 len = 0)
   {
      if(data)
         dMemcpy(chunkData, data, len);
      chunkLen = len;
   }
   virtual void pack(NetConnection *, BitStream *bstream)
   {
      bstream->writeRangedU32(chunkLen, 0, ChunkSize);
      bstream->write(chunkLen, chunkData);
   }
   virtual void write(NetConnection *, BitStream *bstream)
   {
      bstream->writeRangedU32(chunkLen, 0, ChunkSize);
      bstream->write(chunkLen, chunkData);
   }
   virtual void unpack(NetConnection *, BitStream *bstream)
   {
      chunkLen = bstream->readRangedU32(0, ChunkSize);
      bstream->read(chunkLen, chunkData);
   }
   virtual void process(NetConnection *connection)
   {
      connection->chunkReceived(chunkData, chunkLen);
   }
   virtual void notifyDelivered(NetConnection *nc, bool madeIt)
   {
      if(!nc->isRemoved())
        nc->sendFileChunk();
   }
   DECLARE_CONOBJECT(FileChunkEvent);
};
IMPLEMENT_CO_NETEVENT_V1(FileChunkEvent);

void NetConnection::sendFileChunk()
{
   U8 buffer[FileChunkEvent::ChunkSize];
   U32 len = FileChunkEvent::ChunkSize;
   if(len + mCurrentFileBufferOffset > mCurrentFileBufferSize)
      len = mCurrentFileBufferSize - mCurrentFileBufferOffset;
   if(!len)
   {
      ResourceManager->closeStream(mCurrentDownloadingFile);
      mCurrentDownloadingFile = NULL;
      return;
   }
   mCurrentFileBufferOffset += len;
   mCurrentDownloadingFile->read(len, buffer);
   postNetEvent(new FileChunkEvent(buffer, len));
}

bool NetConnection::startSendingFile(const char *fileName)
{
   if(!fileName)
   {
      sendConnectionMessage(SendNextDownloadRequest);
      return false;
   }
   mCurrentDownloadingFile = ResourceManager->openStream(fileName);
   if(!mCurrentDownloadingFile)
   {
      // the server didn't have the file, so send a 0 byte chunk:
      Con::printf("No such file %s.", fileName);
      postNetEvent(new FileChunkEvent(NULL, 0));
      return false;
   }
   Con::printf("Sending file %s.", fileName);
   mCurrentFileBufferSize = mCurrentDownloadingFile->getStreamSize();
   mCurrentFileBufferOffset = 0;
   
   // always have 32 file chunks (64 bytes each) in transit
   sendConnectionMessage(FileDownloadSizeMessage, mCurrentFileBufferSize);
   for(U32 i = 0; i < 32; i++)
      sendFileChunk();
   return true;
}

void NetConnection::sendNextFileDownloadRequest()
{
   // see if we've already downloaded this file...
   while(mMissingFileList.size() && ResourceManager->find(mMissingFileList[0]))
   {
      dFree(mMissingFileList[0]);
      mMissingFileList.pop_front();
   }
   if(mMissingFileList.size())
   {
      postNetEvent(new FileDownloadRequestEvent(&mMissingFileList));
   }
   else
   {
      fileDownloadSegmentComplete();
   }
}


void NetConnection::chunkReceived(U8 *chunkData, U32 chunkLen)
{
   if(chunkLen == 0)
   {
      // the server didn't have the file... apparently it's one we don't need...
      dFree(mCurrentFileBuffer);
      mCurrentFileBuffer = NULL;
      dFree(mMissingFileList[0]);
      mMissingFileList.pop_front();
      return;
   }
   if(chunkLen + mCurrentFileBufferOffset > mCurrentFileBufferSize)
   {
      setLastError("Invalid file chunk from server.");
      return;
   }
   dMemcpy(((U8 *) mCurrentFileBuffer) + mCurrentFileBufferOffset, chunkData, chunkLen);
   mCurrentFileBufferOffset += chunkLen;
   if(mCurrentFileBufferOffset == mCurrentFileBufferSize)
   {
      // this file's done...
      // save it to disk:
      FileStream stream;

      Con::printf("Saving file %s.", mMissingFileList[0]);
      if(!ResourceManager->openFileForWrite(stream, mMissingFileList[0]))
      {
         setLastError("Couldn't open file downloaded by server.");
         return;
      }
      dFree(mMissingFileList[0]);
      mMissingFileList.pop_front();
      stream.write(mCurrentFileBufferSize, mCurrentFileBuffer);
      stream.close();
      mNumDownloadedFiles++;
      dFree(mCurrentFileBuffer);
      mCurrentFileBuffer = NULL;
      sendNextFileDownloadRequest();
   }
   else
   {
      Con::executef(4, "onFileChunkReceived", mMissingFileList[0], Con::getIntArg(mCurrentFileBufferOffset), Con::getIntArg(mCurrentFileBufferSize));
   }
}

