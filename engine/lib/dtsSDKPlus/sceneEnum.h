//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef APP_SCENEENUM_H_
#define APP_SCENEENUM_H_

namespace DTS
{

   class AppSceneEnum
   {
   protected:
      std::vector<AppMesh*> meshes;
      std::vector<AppMesh*> skins;
      std::vector<AppNode*> subTrees;
      std::vector<AppSequence*> sequences;
      AppNode * boundsNode;

      ShapeMimic shapeMimic;
    
      void setExportError(const char * errStr) { AppConfig::SetExportError(errStr); }
      const char * getError() { return AppConfig::GetExportError(); }
      bool isError() { return AppConfig::IsExportError(); }

      void processNode(AppNode *);

   public:
      AppSceneEnum();
      ~AppSceneEnum();

      virtual void enumScene() = 0;      
      Shape * processScene();
   };

}; // namespace DTS

#endif // #define APP_SCENEENUM_H_
