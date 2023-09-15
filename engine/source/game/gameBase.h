//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GAMEBASE_H_
#define _GAMEBASE_H_

#ifndef _SCENEOBJECT_H_
#include "sim/sceneObject.h"
#endif
#ifndef _RESMANAGER_H_
#include "core/resManager.h"
#endif
#ifndef _GTEXMANAGER_H_
#include "dgl/gTexManager.h"
#endif

class NetConnection;
class ProcessList;
struct Move;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

/// Scriptable, demo-able datablock.
///
/// This variant of SimDataBlock performs these additional tasks:
///   - Linking datablock's namepsaces to the namespace of their C++ class, so
///     that datablocks can expose script functionality.
///   - Linking datablocks to a user defined scripting namespace, by setting the
///     className field at datablock definition time.
///   - Adds a category field; this is used by the world creator in the editor to
///     classify creatable shapes. Creatable shapes are placed under the Shapes
///     node in the treeview for this; additional levels are created, named after
///     the category fields.
///   - Adds support for demo stream recording. This support takes the form
///     of the member variable packed. When a demo is being recorded by a client,
///     data is unpacked, then packed again to the data stream, then, in the case
///     of datablocks, preload() is called to process the data. It is occasionally
///     the case that certain references in the datablock stream cannot be resolved
///     until preload is called, in which case a raw ID field is stored in the variable
///     which will eventually be used to store a pointer to the object. However, if
///     packData() is called before we resolve this ID, trying to call getID() on the
///     objecct ID would be a fatal error. Therefore, in these cases, we test packed;
///     if it is true, then we know we have to write the raw data, instead of trying
///     to resolve an ID.
///
/// @see SimDataBlock for further details about datablocks.
/// @see http://hosted.tribalwar.com/t2faq/datablocks.shtml for an excellent 
///      explanation of the basics of datablocks from a scripting perspective.
/// @nosubgrouping
struct GameBaseData : public SimDataBlock {
  private:
   typedef SimDataBlock Parent;

  public:
   bool packed;
   StringTableEntry category;
   StringTableEntry className;

   bool onAdd();
   
   // The derived class should provide the following:
   DECLARE_CONOBJECT(GameBaseData);
   GameBaseData();
   static void initPersistFields();
   bool preload(bool server, char errorBuffer[256]);
   void unpackData(BitStream* stream);
};

//----------------------------------------------------------------------------
class GameConnection;

// For truly it is written: "The wise man extends GameBase for his purposes,
// while the fool has the ability to eject shell casings from the belly of his
// dragon." -- KillerBunny

/// Base class for game objects which use datablocks, networking, are editable, 
/// and need to process ticks.
///
/// @section GameBase_process GameBase and ProcessList
///
/// GameBase adds two kinds of time-based updates. Torque works off of a concept
/// of ticks. Ticks are slices of time 32 milliseconds in length. There are three 
/// methods which are used to update GameBase objects that are registered with
/// the ProcessLists:
///      - processTick(Move*) is called on each object once for every tick, regardless
///        of the "real" framerate.
///      - interpolateTick(float) is called on client objects when they need to interpolate
///        to match the next tick.
///      - advanceTime(float) is called on client objects so they can do time-based behaviour,
///        like updating animations.
///
/// Torque maintains a server and a client processing list; in a local game, both
/// are populated, while in multiplayer situations, either one or the other is
/// populated.
///
/// You can control whether an object is considered for ticking by means of the
/// setProcessTick() method.
///
/// @section GameBase_datablock GameBase and Datablocks
///
/// GameBase adds support for datablocks. Datablocks are secondary classes which store
/// static data for types of game elements. For instance, this means that all "light human
/// male armor" type Players share the same datablock. Datablocks typically store not only
/// raw data, but perform precalculations, like finding nodes in the game model, or 
/// validating movement parameters.
/// 
/// There are three parts to the datablock interface implemented in GameBase:
///      - <b>getDataBlock()</b>, which gets a pointer to the current datablock. This is
///        mostly for external use; for in-class use, it's better to directly access the
///        mDataBlock member.
///      - <b>setDataBlock()</b>, which sets mDataBlock to point to a new datablock; it
///        uses the next part of the interface to inform subclasses of this.
///      - <b>onNewDataBlock()</b> is called whenever a new datablock is assigned to a GameBase.
///
/// Datablocks are also usable through the scripting language.
///
/// @see SimDataBlock for more details.
///
/// @section GameBase_networking GameBase and Networking
///
/// writePacketData() and readPacketData() are called to transfer information needed for client
/// side prediction. They are usually used when updating a client of its control object state. 
///
/// Subclasses of GameBase usually transmit positional and basic status data in the packUpdate()
/// functions, while giving velocity, momentum, and similar state information in the writePacketData().
///
/// writePacketData()/readPacketData() are called <i>in addition</i> to packUpdate/unpackUpdate().
///
/// @nosubgrouping
class GameBase : public SceneObject
{
  private:
   typedef SceneObject Parent;
   friend class ProcessList;

   /// @name Datablock
   /// @{
  private:
   GameBaseData*     mDataBlock;
   StringTableEntry  mNameTag;

   /// @}

   /// @name Tick Processing Internals
   /// @{
  private:
   void plUnlink();
   void plLinkAfter(GameBase*);
   void plLinkBefore(GameBase*);
   void plJoin(GameBase*);
   struct Link {
      GameBase *next;
      GameBase *prev;
   };
   U32  mProcessTag;                      ///< Tag used to sort objects for processing.
   Link mProcessLink;                     ///< Ordered process queue link.
   SimObjectPtr<GameBase> mAfterObject;
   /// @}

  public:
   static bool gShowBoundingBox;    ///< Should we render bounding boxes?
  protected:
   bool mProcessTick;
   
  public:
   GameBase();
   ~GameBase();

   enum GameBaseMasks {
      InitialUpdateMask =     Parent::NextFreeMask,
      DataBlockMask =         InitialUpdateMask << 1,
      ExtendedInfoMask =      DataBlockMask << 1,
      NextFreeMask =          ExtendedInfoMask << 1
   };

   /// @name Inherited Functionality.
   /// @{

   bool onAdd();
   void onRemove();
   void inspectPostApply();
   static void initPersistFields();
   static void consoleInit();
   /// @}

   ///@name Datablock
   ///@{
      
   /// Assigns this object a datablock and loads attributes with onNewDataBlock.
   ///
   /// @see onNewDataBlock
   /// @param   dptr   Datablock
   bool          setDataBlock(GameBaseData* dptr);
   
   /// Returns the datablock for this object.
   GameBaseData* getDataBlock()  { return mDataBlock; }
   
   /// Called when a new datablock is set. This allows subclasses to
   /// appropriately handle new datablocks.
   ///
   /// @see    setDataBlock()
   /// @param  dptr   New datablock
   virtual bool  onNewDataBlock(GameBaseData* dptr);
   ///@}

   /// @name Script
   /// The scriptOnXX methods are invoked by the leaf classes
   /// @{
      
   /// Executes the 'onAdd' script function for this object.
   /// @note This must be called after everything is ready
   void scriptOnAdd();
   
   /// Executes the 'onNewDataBlock' script function for this object.
   ///
   /// @note This must be called after everything is loaded.
   void scriptOnNewDataBlock();
   
   /// Executes the 'onRemove' script function for this object.
   /// @note This must be called while the object is still valid
   void scriptOnRemove();
   /// @}

   /// @name Tick Processing
   /// @{
      
   /// Set the status of tick processing.
   ///
   /// If this is set to true, processTick will be called; if false,
   /// then it will be skipped.
   ///
   /// @see processTick
   /// @param   t   If true, tick processing is enabled.
   void setProcessTick(bool t) { mProcessTick = t; }
   
   /// Force this object to process after some other object.
   ///
   /// For example, a player mounted to a vehicle would want to process after the vehicle, 
   /// to prevent a visible "lagging" from occuring when the vehicle motions, so the player
   /// would be set to processAfter(theVehicle);
   ///
   /// @param   obj   Object to process after
   void processAfter(GameBase *obj);
   
   /// Clears the effects of a call to processAfter()
   void clearProcessAfter();
   
   /// Returns the object that this processes after.
   ///
   /// @see processAfter
   GameBase* getProcessAfter() { return mAfterObject; }
   
   /// Removes this object from the tick-processing list
   void removeFromProcessList() { plUnlink(); }
   
   /// Processes a move event and updates object state once every 32 milliseconds.
   ///
   /// This takes place both on the client and server, every 32 milliseconds (1 tick).
   ///
   /// @see    ProcessList
   /// @param  move   Move event corresponding to this tick, or NULL.
   virtual void processTick(const Move *move);
   
   /// Interpolates between tick events.  This takes place on the CLIENT ONLY.
   ///
   /// @param   delta   Time since last call to interpolate
   virtual void interpolateTick(F32 delta);
   
   /// Advances simulation time for animations. This is called every frame.
   ///
   /// @param   dt   Time since last advance call
   virtual void advanceTime(F32 dt);
   /// @}

   /// Returns the velocity of this object.
   virtual Point3F getVelocity() const;

   /// @name Network
   /// @see NetObject, NetConnection
   /// @{

   F32  getUpdatePriority(CameraScopeQuery *focusObject, U32 updateMask, S32 updateSkips);
   U32  packUpdate  (NetConnection *conn, U32 mask, BitStream *stream);
   void unpackUpdate(NetConnection *conn,           BitStream *stream);
   
   /// Write state information necessary to perform client side prediction of an object.
   ///
   /// This information is sent only to the controling object. For example, if you are a client 
   /// controlling a Player, the server uses writePacketData() instead of packUpdate() to
   /// generate the data you receive.
   ///
   /// @param   conn     Connection for which we're generating this data.
   /// @param   stream   Bitstream for output.
   virtual void writePacketData(GameConnection *conn, BitStream *stream);
   
   /// Read data written with writePacketData() and update the object state.
   ///
   /// @param   conn    Connection for which we're generating this data.
   /// @param   stream  Bitstream to read.
   virtual void readPacketData(GameConnection *conn, BitStream *stream);
   
   /// Gets the checksum for packet data.
   /// 
   /// Basically writes a packet, does a CRC check on it, and returns 
   /// that CRC.
   ///
   /// @see writePacketData
   /// @param   conn   Game connection
   virtual U32 getPacketDataChecksum(GameConnection *conn);
   ///@}

   DECLARE_CONOBJECT(GameBase);
};


//-----------------------------------------------------------------------------

#define TickShift   5
#define TickMs      (1 << TickShift)
#define TickSec     (F32(TickMs) / 1000.0f)
#define TickMask    (TickMs - 1)

/// List to keep track of GameBases to process.
class ProcessList
{
   GameBase head;
   U32 mCurrentTag;
   SimTime mLastTick;
   SimTime mLastTime;
   SimTime mLastDelta;
   bool mIsServer;
   bool mDirty;
   static bool mDebugControlSync;

   void orderList();
   void advanceObjects();

public:
   SimTime getLastTime() { return mLastTime; }
   ProcessList(bool isServer);
   void markDirty()  { mDirty = true; }
   bool isDirty()  { return mDirty; }
   void addObject(GameBase* obj) {
      obj->plLinkBefore(&head);
   }
   F32 getLastInterpDelta() { return mLastDelta / F32(TickMs); }

   /// @name Advancing Time
   /// The advance time functions return true if a tick was processed.
   ///
   /// These functions go through either gServerProcessList or gClientProcessList and
   /// call each GameBase's processTick().
   /// @{

   bool advanceServerTime(SimTime timeDelta);
   bool advanceClientTime(SimTime timeDelta);

   /// @}
};

extern ProcessList gClientProcessList;
extern ProcessList gServerProcessList;

#endif
