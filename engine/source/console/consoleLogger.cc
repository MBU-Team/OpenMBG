//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "console/consoleLogger.h"
#include "console/consoleTypes.h"

Vector<ConsoleLogger *> ConsoleLogger::mActiveLoggers;

IMPLEMENT_CONOBJECT( ConsoleLogger );

//-----------------------------------------------------------------------------

ConsoleLogger::ConsoleLogger() {
   mInitalized = false;
   mFilename = NULL;
   mLogging = false;
   mAppend = false;
}

//-----------------------------------------------------------------------------

ConsoleLogger::ConsoleLogger( const char *fileName, bool append ) {
   mInitalized = false;
   mLogging = false;

   mLevel = ConsoleLogEntry::Normal;
   mFilename = StringTable->insert( fileName );
   mAppend = append;

   init();   
}

//-----------------------------------------------------------------------------

static EnumTable::Enums logLevelEnums[] = {
	{ ConsoleLogEntry::Normal,     "normal"  },
   { ConsoleLogEntry::Warning,    "warning" },
	{ ConsoleLogEntry::Error,      "error"   },
};

static EnumTable gLogLevelTable( 3, &logLevelEnums[0] ); 

void ConsoleLogger::initPersistFields() {
   Parent::initPersistFields();

   addGroup( "Logging" );
   addField( "level",   TypeEnum,     Offset( mLevel,    ConsoleLogger ), 1, &gLogLevelTable );
   endGroup( "Logging" );
}

//-----------------------------------------------------------------------------

bool ConsoleLogger::processArguments( S32 argc, const char **argv ) {
   if( argc == 0 )
      return false;

   bool append = false;
   
   if( argc == 2 )
      append = dAtob( argv[1] );

   mAppend = append;
   mFilename = StringTable->insert( argv[0] );

   if( init() ) {
      attach();
      return true;
   }

   return false;
}

//-----------------------------------------------------------------------------

ConsoleLogger::~ConsoleLogger() {
   detach();
}

//-----------------------------------------------------------------------------

bool ConsoleLogger::init() {
   if( mInitalized )
      return false;

   if( mFilename == NULL )
      return false;

   Con::addConsumer( ConsoleLogger::logCallback );
   mInitalized = true;

   return true;
}

//-----------------------------------------------------------------------------

bool ConsoleLogger::attach() {
   // Check to see if this is intalized before using it
   if( !mInitalized ) {
      if( !init() ) {
         Con::errorf( "ConsoleLogger failed to initalize." );
         return false;
      }
   }
      
   if( mLogging )
      return false;

   // Open the filestream
   mStream.open( mFilename, ( mAppend ? FileStream::WriteAppend : FileStream::Write ) );

   // Add this to list of active loggers
   mActiveLoggers.push_back( this );
   mLogging = true;

   return true;
}

//-----------------------------------------------------------------------------

bool ConsoleLogger::detach() {
   
   // Make sure this is valid before messing with it
   if( !mInitalized ) {
      if( !init() ) {
         return false;
      }
   }

   if( !mLogging )
      return false;

   // Close filestream
   mStream.close();

   // Remove this object from the list of active loggers
   for( int i = 0; i < mActiveLoggers.size(); i++ ) {
      if( mActiveLoggers[i] == this ) {
         mActiveLoggers.erase( i );
         mLogging = false;
         return true;
      }
   }

   return false; // If this happens, it's bad...
}

//-----------------------------------------------------------------------------

void ConsoleLogger::logCallback( ConsoleLogEntry::Level level, const char *consoleLine ) {

   ConsoleLogger *curr;

   // Loop through active consumers and send them the message
   for( int i = 0; i < mActiveLoggers.size(); i++ ) {
      curr = mActiveLoggers[i];

      // If the log level is within the log threshhold, log it
      if( curr->mLevel <= level )
         curr->log( consoleLine );
   }
}

//-----------------------------------------------------------------------------

void ConsoleLogger::log( const char *consoleLine ) {
   // Check to see if this is intalized before using it
   if( !mInitalized ) {
      if( !init() ) {
         Con::errorf( "I don't know how this happened, but log called on this without it being initialized" );
         return;
      }
   }

   mStream.writeLine( (U8 *)consoleLine );
}

//-----------------------------------------------------------------------------

ConsoleMethod( ConsoleLogger, attach, bool, 2, 2, "() Attaches this object to the console and begins logging" ) {
   ConsoleLogger *logger = static_cast<ConsoleLogger *>( object );
   return logger->attach();
}

//-----------------------------------------------------------------------------

ConsoleMethod( ConsoleLogger, detach, bool, 2, 2, "() Detaches this object from the console and stops logging" ) {
   ConsoleLogger *logger = static_cast<ConsoleLogger *>( object );
   return logger->detach();
}
