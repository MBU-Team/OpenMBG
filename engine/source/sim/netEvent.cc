//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "core/dnet.h"
#include "console/simBase.h"
#include "sim/netConnection.h"
#include "core/bitStream.h"

#define DebugChecksum 0xF00DBAAD

FreeListChunker<NetEventNote> NetConnection::mEventNoteChunker;

void NetEvent::notifyDelivered(NetConnection *, bool)
{
}

void NetEvent::notifySent(NetConnection *)
{
}

#ifdef DEBUG_NET
const char *NetEvent::getDebugName()
{
   return getClassName();
}
#endif

void NetConnection::eventOnRemove()
{
   while(mNotifyEventList)
   {
      NetEventNote *temp = mNotifyEventList;
      mNotifyEventList = temp->mNextEvent;
      
      temp->mEvent->notifyDelivered(this, true);
      temp->mEvent->decRef();
      mEventNoteChunker.free(temp);
   }

   while(mUnorderedSendEventQueueHead)
   {
      NetEventNote *temp = mUnorderedSendEventQueueHead;
      mUnorderedSendEventQueueHead = temp->mNextEvent;
      
      temp->mEvent->notifyDelivered(this, true);
      temp->mEvent->decRef();
      mEventNoteChunker.free(temp);
   }

   while(mSendEventQueueHead)
   {
      NetEventNote *temp = mSendEventQueueHead;
      mSendEventQueueHead = temp->mNextEvent;
      
      temp->mEvent->notifyDelivered(this, true);
      temp->mEvent->decRef();
      mEventNoteChunker.free(temp);
   }
}

void NetConnection::eventPacketDropped(PacketNotify *notify)
{
   NetEventNote *walk = notify->eventList;
   NetEventNote **insertList = &mSendEventQueueHead;
   NetEventNote *temp;
   
   while(walk)
   {
      switch(walk->mEvent->mGuaranteeType)
      {
         // It was a guaranteed ordered packet, reinsert it back into
         // mSendEventQueueHead in the right place (based on seq numbers)
         case NetEvent::GuaranteedOrdered:

            //Con::printf("EVT  %d: DROP - %d", getId(), walk->mSeqCount);
            
            while(*insertList && (*insertList)->mSeqCount < walk->mSeqCount)
               insertList = &((*insertList)->mNextEvent);
            
            temp = walk->mNextEvent;
            walk->mNextEvent = *insertList;
            if(!walk->mNextEvent)
               mSendEventQueueTail = walk;
            *insertList = walk;
            insertList = &(walk->mNextEvent);
            walk = temp;
            break;

         // It was a guaranteed packet, put it at the top of
         // mUnorderedSendEventQueueHead.
         case NetEvent::Guaranteed:
            temp = walk->mNextEvent;
            walk->mNextEvent = mUnorderedSendEventQueueHead;
            mUnorderedSendEventQueueHead = walk;
            if(!walk->mNextEvent)
               mUnorderedSendEventQueueTail = walk;
            walk = temp;
            break;

         // Or else it was an unguaranteed packet, notify that
         // it was _not_ delivered and blast it.
         case NetEvent::Unguaranteed:
            walk->mEvent->notifyDelivered(this, false);
            walk->mEvent->decRef();
            temp = walk->mNextEvent;
            mEventNoteChunker.free(walk);
            walk = temp;
      }
   }
}

void NetConnection::eventPacketReceived(PacketNotify *notify)
{
   NetEventNote *walk = notify->eventList;
   NetEventNote **noteList = &mNotifyEventList;

   while(walk)
   {
      NetEventNote *next = walk->mNextEvent;
      if(walk->mEvent->mGuaranteeType != NetEvent::GuaranteedOrdered)
      {
         walk->mEvent->notifyDelivered(this, true);
         walk->mEvent->decRef();
         mEventNoteChunker.free(walk);
         walk = next;
      }
      else
      {
         while(*noteList && (*noteList)->mSeqCount < walk->mSeqCount)
            noteList = &((*noteList)->mNextEvent);
         
         walk->mNextEvent = *noteList;
         *noteList = walk;
         noteList = &walk->mNextEvent;
         walk = next;
      }
   }
   while(mNotifyEventList && mNotifyEventList->mSeqCount == mLastAckedEventSeq + 1)
   {
      mLastAckedEventSeq++;
      NetEventNote *next = mNotifyEventList->mNextEvent;
      //Con::printf("EVT  %d: ACK - %d", getId(), mNotifyEventList->mSeqCount);
      mNotifyEventList->mEvent->notifyDelivered(this, true);
      mNotifyEventList->mEvent->decRef();
      mEventNoteChunker.free(mNotifyEventList);
      mNotifyEventList = next;
   }
}

void NetConnection::eventWritePacket(BitStream *bstream, PacketNotify *notify)
{
#ifdef DEBUG_NET
   bstream->writeInt(DebugChecksum, 32);
#endif

   NetEventNote *packQueueHead = NULL, *packQueueTail = NULL;

   while(mUnorderedSendEventQueueHead)
   {
      if(bstream->isFull())
         break;
      // dequeue the first event
      NetEventNote *ev = mUnorderedSendEventQueueHead;
      mUnorderedSendEventQueueHead = ev->mNextEvent;
      U32 start = bstream->getCurPos();
      
      bstream->writeFlag(true);
      S32 classId = ev->mEvent->getClassId(getNetClassGroup());
      bstream->writeClassId(classId, NetClassTypeEvent, getNetClassGroup());

      ev->mEvent->pack(this, bstream);
      DEBUG_LOG(("PKLOG %d EVENT %d: %s", getId(), bstream->getCurPos() - start, ev->mEvent->getDebugName()) );

#ifdef DEBUG_NET
      bstream->writeInt(classId ^ DebugChecksum, 32);
#endif
      // add this event onto the packet queue
      ev->mNextEvent = NULL;
      if(!packQueueHead)
         packQueueHead = ev;
      else
         packQueueTail->mNextEvent = ev;
      packQueueTail = ev;
   }
   
   bstream->writeFlag(false);   
   S32 prevSeq = -2;
   
   while(mSendEventQueueHead)
   {
      if(bstream->isFull())
         break;
      
      // if the event window is full, stop processing
      if(mSendEventQueueHead->mSeqCount > mLastAckedEventSeq + 126)
         break;

      // dequeue the first event
      NetEventNote *ev = mSendEventQueueHead;
      mSendEventQueueHead = ev->mNextEvent;
      
      //Con::printf("EVT  %d: SEND - %d", getId(), ev->mSeqCount);

      bstream->writeFlag(true);

      ev->mNextEvent = NULL;
      if(!packQueueHead)
         packQueueHead = ev;
      else
         packQueueTail->mNextEvent = ev;
      packQueueTail = ev;
      if(!bstream->writeFlag(ev->mSeqCount == prevSeq + 1))
         bstream->writeInt(ev->mSeqCount, 7);
      
      prevSeq = ev->mSeqCount;

      U32 start = bstream->getCurPos();
      S32 classId = ev->mEvent->getClassId(getNetClassGroup());
      bstream->writeClassId(classId, NetClassTypeEvent, getNetClassGroup());
      ev->mEvent->pack(this, bstream);
      DEBUG_LOG(("PKLOG %d EVENT %d: %s", getId(), bstream->getCurPos() - start, ev->mEvent->getDebugName()) );
#ifdef DEBUG_NET
      bstream->writeInt(classId ^ DebugChecksum, 32);
#endif
   }
   for(NetEventNote *ev = packQueueHead; ev; ev = ev->mNextEvent)
      ev->mEvent->notifySent(this);
      
   notify->eventList = packQueueHead;
   bstream->writeFlag(0);
}

void NetConnection::eventReadPacket(BitStream *bstream)
{
#ifdef DEBUG_NET
   U32 sum = bstream->readInt(32);
   AssertISV(sum == DebugChecksum, "Invalid checksum.");
#endif
   
   S32 prevSeq = -2;
   NetEventNote **waitInsert = &mWaitSeqEvents;
   bool unguaranteedPhase = true;
   
   while(true)
   {
      bool bit = bstream->readFlag();
      if(unguaranteedPhase && !bit)
      {
         unguaranteedPhase = false;
         bit = bstream->readFlag();
      }
      if(!unguaranteedPhase && !bit)
         break;
      
      S32 seq = -1;
      
      if(!unguaranteedPhase) // get the sequence
      {
         if(bstream->readFlag())
            seq = (prevSeq + 1) & 0x7f;
         else
            seq = bstream->readInt(7);
         prevSeq = seq;
      }
      S32 classId = bstream->readClassId(NetClassTypeEvent, getNetClassGroup());
      if(classId == -1)
      {
         setLastError("Invalid packet.");
         return;
      }
      NetEvent *evt = (NetEvent *) ConsoleObject::create(getNetClassGroup(), NetClassTypeEvent, classId);
      if(!evt)
      {
         setLastError("Invalid packet.");
         return;
      }
      AbstractClassRep *rep = evt->getClassRep();
      if((rep->mNetEventDir == NetEventDirServerToClient && !isServerConnection())
         || (rep->mNetEventDir == NetEventDirClientToServer && isServerConnection()) )
      {
         setLastError("Invalid Packet.");
         return;
      }


      evt->mSourceId = getId();
      evt->unpack(this, bstream);
      if(mErrorBuffer[0])
         return;
#ifdef DEBUG_NET
      U32 checksum = bstream->readInt(32);
      AssertISV( (checksum ^ DebugChecksum) == (U32)classId,
         avar("unpack did not match pack for event of class %s.",
            evt->getClassName()) );
#endif
      if(unguaranteedPhase)
      {
         evt->process(this);
         delete evt;
         if(mErrorBuffer[0])
            return;
         continue;
      }
      seq |= (mNextRecvEventSeq & ~0x7F);
      if(seq < mNextRecvEventSeq)
         seq += 128;

      NetEventNote *note = mEventNoteChunker.alloc();
      note->mEvent = evt;
      note->mEvent->incRef();

      note->mSeqCount = seq;
      //Con::printf("EVT  %d: RECV - %d", getId(), evt->mSeqCount);
      while(*waitInsert && (*waitInsert)->mSeqCount < seq)
         waitInsert = &((*waitInsert)->mNextEvent);
      
      note->mNextEvent = *waitInsert;
      *waitInsert = note;
      waitInsert = &(note->mNextEvent);
   }
   while(mWaitSeqEvents && mWaitSeqEvents->mSeqCount == mNextRecvEventSeq)
   {
      mNextRecvEventSeq++;
      NetEventNote *temp = mWaitSeqEvents;
      mWaitSeqEvents = temp->mNextEvent;
      
      //Con::printf("EVT  %d: PROCESS - %d", getId(), temp->mSeqCount);
      temp->mEvent->process(this);
      temp->mEvent->decRef();
      mEventNoteChunker.free(temp);
      if(mErrorBuffer[0])
         return;
   }
}

bool NetConnection::postNetEvent(NetEvent *theEvent)
{
   if(!mSendingEvents)
   {
      delete theEvent;
      return false;
   }
   NetEventNote *event = mEventNoteChunker.alloc();
   event->mEvent = theEvent;
   theEvent->incRef();

   event->mNextEvent = NULL;
   if(theEvent->mGuaranteeType == NetEvent::GuaranteedOrdered)
   {
      event->mSeqCount = mNextSendEventSeq++;
      if(!mSendEventQueueHead)
         mSendEventQueueHead = event;
      else
         mSendEventQueueTail->mNextEvent = event;
      mSendEventQueueTail = event;
   }
   else
   {
      event->mSeqCount = InvalidSendEventSeq;
      if(!mUnorderedSendEventQueueHead)
         mUnorderedSendEventQueueHead = event;
      else
         mUnorderedSendEventQueueTail->mNextEvent = event;
      mUnorderedSendEventQueueTail = event;
   }
   return true;
}


void NetConnection::eventWriteStartBlock(ResizeBitStream *stream)
{
   stream->write(mNextRecvEventSeq);
   for(NetEventNote *walk = mWaitSeqEvents; walk; walk = walk->mNextEvent)
   {
      stream->writeFlag(true);
      S32 classId = walk->mEvent->getClassId(getNetClassGroup());
      stream->writeClassId(classId, NetClassTypeEvent, getNetClassGroup());
      walk->mEvent->write(this, stream);
      stream->validate();
   }
   stream->writeFlag(false);
}

void NetConnection::eventReadStartBlock(BitStream *stream)
{
   stream->read(&mNextRecvEventSeq);
   
   NetEventNote *lastEvent = NULL;
   while(stream->readFlag())
   {
      S32 classTag = stream->readClassId(NetClassTypeEvent, getNetClassGroup());
      NetEvent *evt = (NetEvent *) ConsoleObject::create(getNetClassGroup(), NetClassTypeEvent, classTag);
      evt->unpack(this, stream);
      NetEventNote *add = mEventNoteChunker.alloc();
      add->mEvent = evt;
      evt->incRef();
      add->mNextEvent = NULL;
      
      if(!lastEvent)
         mWaitSeqEvents = add;
      else
         lastEvent->mNextEvent = add;
      lastEvent = add;
   }
}
