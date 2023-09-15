/**********************************************************************
 *<
   FILE: max2dtsExporterPlus.cpp

   DESCRIPTION:   Appwizard generated plugin

   CREATED BY: 

   HISTORY: 

 *>   Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/

#include "max2dtsExporterPlus.h"
#include "maxSceneEnum.h"
#include "appConfig.h"
#include <fstream>


#define MAX2DTSEXPORTERPLUS_CLASS_ID   Class_ID(0x612c5c7, 0x5924d160)

class Max2dtsExporterPlus : public SceneExport {
   public:


      static HWND hParams;


      int            ExtCount();               // Number of extensions supported
      const TCHAR *   Ext(int n);               // Extension #n (i.e. "3DS")
      const TCHAR *   LongDesc();               // Long ASCII description (i.e. "Autodesk 3D Studio File")
      const TCHAR *   ShortDesc();            // Short ASCII description (i.e. "3D Studio")
      const TCHAR *   AuthorName();            // ASCII Author name
      const TCHAR *   CopyrightMessage();         // ASCII Copyright message
      const TCHAR *   OtherMessage1();         // Other message #1
      const TCHAR *   OtherMessage2();         // Other message #2
      unsigned int   Version();               // Version number * 100 (i.e. v3.01 = 301)
      void         ShowAbout(HWND hWnd);      // Show DLL's "About..." box

      BOOL SupportsOptions(int ext, DWORD options);
      int            DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts=FALSE, DWORD options=0);

      void alert(const char * title, const char * message, BOOL suppressPrompts);
      
      //Constructor/Destructor

      Max2dtsExporterPlus();
      ~Max2dtsExporterPlus();      

};


class Max2dtsExporterPlusClassDesc:public ClassDesc2 {
   public:
   int          IsPublic() { return TRUE; }
   void *         Create(BOOL loading = FALSE) { return new Max2dtsExporterPlus(); }
   const TCHAR *   ClassName() { return GetString(IDS_CLASS_NAME); }
   SClass_ID      SuperClassID() { return SCENE_EXPORT_CLASS_ID; }
   Class_ID      ClassID() { return MAX2DTSEXPORTERPLUS_CLASS_ID; }
   const TCHAR*    Category() { return GetString(IDS_CATEGORY); }

   const TCHAR*   InternalName() { return _T("Max2dtsExporterPlus"); }   // returns fixed parsable name (scripter-visible name)
   HINSTANCE      HInstance() { return hInstance; }            // returns owning module handle

};



static Max2dtsExporterPlusClassDesc Max2dtsExporterPlusDesc;
ClassDesc2* GetMax2dtsExporterPlusDesc() { return &Max2dtsExporterPlusDesc; }


BOOL CALLBACK Max2dtsExporterPlusOptionsDlgProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {
   static Max2dtsExporterPlus *imp = NULL;

   switch(message) {
      case WM_INITDIALOG:
         imp = (Max2dtsExporterPlus *)lParam;
         CenterWindow(hWnd,GetParent(hWnd));
         return TRUE;

      case WM_CLOSE:
         EndDialog(hWnd, 0);
         return TRUE;
   }
   return FALSE;
}


//--- Max2dtsExporterPlus -------------------------------------------------------
Max2dtsExporterPlus::Max2dtsExporterPlus()
{

}

Max2dtsExporterPlus::~Max2dtsExporterPlus() 
{

}

int Max2dtsExporterPlus::ExtCount()
{
   // Returns the number of file name extensions supported by the plug-in.
   return 1;
}

const TCHAR *Max2dtsExporterPlus::Ext(int n)
{      
   // Return the 'i-th' file name extension
   return _T("DTS");
}

const TCHAR *Max2dtsExporterPlus::LongDesc()
{
   //Return long ASCII description
   return _T("Torque game engine dts shape file");
}
   
const TCHAR *Max2dtsExporterPlus::ShortDesc() 
{         
   //Return short ASCII description
   return _T("DTS Shape");
}

const TCHAR *Max2dtsExporterPlus::AuthorName()
{         
   //Return ASCII Author name
   return _T("Clark Fagot");
}

const TCHAR *Max2dtsExporterPlus::CopyrightMessage() 
{   
   // Return ASCII Copyright message
   return _T("Copyright GarageGames");
}

const TCHAR *Max2dtsExporterPlus::OtherMessage1() 
{      
   //Return Other message #1 if any
   return _T("");
}

const TCHAR *Max2dtsExporterPlus::OtherMessage2() 
{      
   //Return other message #2 in any
   return _T("");
}

unsigned int Max2dtsExporterPlus::Version()
{            
   //Return Version number * 100 (i.e. v3.01 = 301)
   return 100;
}

void Max2dtsExporterPlus::ShowAbout(HWND hWnd)
{
   // Optional
}

BOOL Max2dtsExporterPlus::SupportsOptions(int ext, DWORD options)
{
   // Decide which options to support.  Simply return
   // true for each option supported by each Extension 
   // the exporter supports.

   return TRUE;
}

void Max2dtsExporterPlus::alert(const char * title, const char * message, BOOL suppressPrompts)
{
   if (suppressPrompts)
      DTS::AppConfig::PrintDump(-1,message);
   else
   {
      TSTR str1(message);
      TSTR str2(title);
      MessageBox(GetActiveWindow(), str1, str2, MB_OK);
   }
}

int Max2dtsExporterPlus::DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts, DWORD options)
{
   // Implement the actual file Export here and 
   // return TRUE If the file is exported properly
   if (!DTS::AppConfig::SetDumpFile(name))
      alert("Dump file error",DTS::avar("Unable to create dumpfile for shape \"%s\".",name),suppressPrompts);

   // Get config file if it exists...
   const char * configBase = i->GetCurFilePath();
   DTS::AppConfig::ReadConfigFile(configBase,".max");

   DTS::MaxSceneEnum maxSceneEnum(ei->theScene);
   DTS::Shape * shape = maxSceneEnum.processScene();
   if (!DTS::AppConfig::IsExportError())
   {
      std::ofstream os;
      os.open(name,std::ofstream::binary);
      shape->save(os);
      os.close();
   }
   DTS::AppConfig::CloseDumpFile();
   delete shape;

   if (DTS::AppConfig::IsExportError())
      alert("Export error",DTS::AppConfig::GetExportError(),suppressPrompts);

   return !DTS::AppConfig::IsExportError();
}


