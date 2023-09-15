//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include <maya/MPxFileTranslator.h>
#include <maya/MObject.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MFnPlugin.h>
#include <fstream>
#include "appConfig.h"
#include "mayaSceneEnum.h"

class exportDTS : public MPxFileTranslator {
public:
	exportDTS ();
	virtual ~exportDTS (); 

	virtual MStatus 	writer (
							const MFileObject &file,
							const MString &optionsString,
							FileAccessMode mode
						);

public:
	virtual bool		haveWriteMethod () const;
	virtual MString 	defaultExtension () const;
	virtual MFileKind	identifyFile (
							const MFileObject &,
							const char *buffer,
							short size
						) const;

	static void *		creator ();
};


//--------------------------------------------------------------------------------
//	Plugin management
//--------------------------------------------------------------------------------

MStatus initializePlugin (MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj, "GarageGames", "0.9.0 beta");
	status = plugin.registerFileTranslator ("exportDTS", "", exportDTS::creator);
	return (status);
}

MStatus uninitializePlugin (MObject obj)
{
	MFnPlugin plugin (obj);
	plugin.deregisterFileTranslator ("exportDTS");
	return (MS::kSuccess);
}


bool exportDTS::haveWriteMethod () const
{
	return (true);
}

MString exportDTS::defaultExtension () const
{
	return (MString("dts"));
}

MPxFileTranslator::MFileKind exportDTS::identifyFile (const MFileObject &file, const char *buffer, short size) const
{
	const char * name = file.name().asChar();
	int nameLength = strlen(name);

	if ((nameLength > 4) && !strcasecmp(name+nameLength-4, ".dts"))
		return (kIsMyFileType);

	return (kNotMyFileType);
}

void * exportDTS::creator ()
{
	return (new exportDTS);
}


//--------------------------------------------------------------------------------
//	Export functionality
//--------------------------------------------------------------------------------

exportDTS::exportDTS ()
{
}

exportDTS::~exportDTS ()
{
}

MStatus exportDTS::writer (const MFileObject &file,const MString &optionsString,FileAccessMode mode)
{
   // start error logging, setup status for failure
	MStatus status = MS::kFailure;
   MGlobal::startErrorLogging();

   MString fileNameStr = file.fullName();
   const char * name = fileNameStr.asChar();

   if (!DTS::AppConfig::SetDumpFile(name))
   {
      MGlobal::doErrorLogEntry(DTS::avar("Unable to create dumpfile for shape \"%s\".",name));
      MGlobal::stopErrorLogging();
      return status;
   }

   // Get config file if it exists...
   MString sourceFile;
   MGlobal::executeCommand("file",sourceFile);
   DTS::AppConfig::ReadConfigFile(sourceFile.asChar(),".mb");

   DTS::MayaSceneEnum mayaSceneEnum;
   DTS::Shape * shape = mayaSceneEnum.processScene();
   if (!DTS::AppConfig::IsExportError())
   {
      std::ofstream os;

      // not sure why maya sometimes doesn't handle this, but on occassion
      // we get multiple dts's in filename if user selects a dts file
      // when saving.  Get rid of the extra dts here.
      char * delMe = NULL;
      char * extraDTS = ".dts.dts";
      if (strlen(name)>strlen(extraDTS) && !stricmp(extraDTS,name + strlen(name) - strlen(extraDTS)))
      {
         delMe = new char [strlen(name)+1];
         delMe[strlen(name)-4] = '\0';
         name = delMe;
      }

      os.open(name,std::ofstream::binary);
      shape->save(os);
      os.close();
      delete [] delMe;
   }
   DTS::AppConfig::CloseDumpFile();
   delete shape;

   if (DTS::AppConfig::IsExportError())
   {
      MGlobal::doErrorLogEntry(DTS::AppConfig::GetExportError());
      const char * cmdPattern = "confirmDialog -t \"Error Exporting DTS Shape\" -m \"%s\"";
      char * confirmDialog = new char[strlen(DTS::AppConfig::GetExportError()) + strlen(cmdPattern) + 1];
      sprintf(confirmDialog,cmdPattern,DTS::AppConfig::GetExportError());
      MGlobal::executeCommand(confirmDialog,true);
      MGlobal::displayError(MString(DTS::AppConfig::GetExportError()));
   }
   else
      status = MS::kSuccess;

   // stop error logging, return status
   MGlobal::stopErrorLogging();

   return status;
}

