//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "platform/platformAssert.h"
#include "dgl/gBitmap.h"
#include "dgl/gPalette.h"
#include "platform/event.h"
#include "core/fileStream.h"
#include "texture2bm8/svector.h"
#include "sim/frameAllocator.h"
#include "dgl/gTexManager.h"
#include "console/console.h"

//------------------------------------------------------------------------------
#include "platform/gameInterface.h"
class TexMungeGame : public GameInterface
{
   public:
      S32 main(S32 argc, const char **argv);
} GameObject;

// FOR SILLY LINK DEPENDANCY
bool gEditingMission = false;
void GameHandleNotify(NetConnectionId, bool)
{
}

#if defined(TORQUE_DEBUG)
   const char * const gProgramVersion = "0.900d-beta";
#else
   const char * const gProgramVersion = "0.900r-beta";
#endif


//------------------------------------------------------------------------------

static bool initLibraries()
{
   // asserts should be created FIRST
   PlatformAssert::create();
   FrameAllocator::init(2 << 20);


   _StringTable::create();

   // Register known file types here
   // ResManager::create();   
   // ResourceManager->registerExtension(".png", constructBitmapPNG);

   Con::init();

   Math::init();
   Platform::init();    // platform specific initialization
   return(true);
}

//------------------------------------------------------------------------------

static void shutdownLibraries()
{
   // shut down
   Platform::shutdown();
   Con::shutdown();

   //ResManager::destroy();
   _StringTable::destroy();

   FrameAllocator::destroy();
   // asserts should be destroyed LAST
   PlatformAssert::destroy();
}


void quantizeRGB(const GBitmap&, GBitmap*);
void quantizeRGBA(const GBitmap&, GBitmap*);

int TexMungeGame::main(S32 argc, const char** argv)
{
   if(!initLibraries())
      return(0);

   // so the debugger can actually do something....
//   if(argc<3)
//   {
//      static const char* argvFake[] = { "blah", "beagle.lmale.png", "beagle.lmale.bm8"};
//      argc = 3;
//      argv = argvFake;
//   }
   
   // info
   if(argc < 3)
   {
      dPrintf("\nTexture2bm8 - Torque palettized texture creator\n"
              "Copyright (C) GarageGames.com, Inc.\n"
              "  Programmer: Dave Moore\n"
              "  Program version: %s\n"
              "  Built: %s at %s\n", gProgramVersion, __DATE__, __TIME__);
      
      dPrintf("\n  Usage: texure2bm8 <input>.png <output>.bm8\n");
      shutdownLibraries();
      return(0);
   }

   GBitmap bmp;
   FileStream fs;
   if (fs.open(argv[1], FileStream::Read) == false) {
      dPrintf("Error: unable to open file: %s for reading\n", argv[1]);
      shutdownLibraries();
      return -1;
   }
   if (bmp.readPNG(fs) == false) {
      dPrintf("Error: unable to read %s as a .PNG\n", argv[1]);
      shutdownLibraries();
      return -1;
   }
   fs.close();
//    if (isPow2(bmp.getHeight()) == false || isPow2(bmp.getWidth()) == false) {
//       dPrintf("Error: dimensions of %s (%d, %d) are not powers of 2.\n",
//               argv[1], bmp.getWidth(), bmp.getHeight());
//       shutdownLibraries();
//       return -1;
//    }
   if (bmp.getFormat() != GBitmap::RGB &&
       bmp.getFormat() != GBitmap::RGBA) {
      dPrintf("Error: %s is not a 24 or 32-bit .PNG\n", argv[1]);
      return false;
   }
   if (isPow2(bmp.getWidth())  &&
       isPow2(bmp.getHeight()) &&
       bmp.getWidth()  > 1     &&
       bmp.getHeight() > 1)
      bmp.extrudeMipLevels();
   
   GBitmap newBMP;
   if (bmp.getFormat() == GBitmap::RGB)
      quantizeRGB(bmp, &newBMP);
   else
      quantizeRGBA(bmp, &newBMP);

   FileStream fws;
   if (fws.open(argv[2], FileStream::Write) == false)
   {
      dPrintf("Error: unable to open file: %s for writing\n", argv[2]);
      shutdownLibraries();
      return -1;
   }

   if (newBMP.writeBmp8(fws) == false)
   {
      dPrintf("Error: couldn't write bitmap as a paletted texture\n");
      shutdownLibraries();
      return -1;
   }
   
   shutdownLibraries();
   return(0);
}

F32 calcDistRGB(GPalette* palette, const U8* pixel, const U32 index)
{
   return ((F32((palette->getColors())[index].red)   - F32(pixel[0])) * (F32((palette->getColors())[index].red)   - F32(pixel[0])) +
           (F32((palette->getColors())[index].green) - F32(pixel[1])) * (F32((palette->getColors())[index].green) - F32(pixel[1])) +
           (F32((palette->getColors())[index].blue)  - F32(pixel[2])) * (F32((palette->getColors())[index].blue)  - F32(pixel[2])));
}

F32 calcDistRGBA(GPalette* palette, const U8* pixel, const U32 index)
{
   return ((F32((palette->getColors())[index].red)   - F32(pixel[0])) * (F32((palette->getColors())[index].red)   - F32(pixel[0])) +
           (F32((palette->getColors())[index].green) - F32(pixel[1])) * (F32((palette->getColors())[index].green) - F32(pixel[1])) +
           (F32((palette->getColors())[index].blue)  - F32(pixel[2])) * (F32((palette->getColors())[index].blue)  - F32(pixel[2])) +
           (F32((palette->getColors())[index].alpha) - F32(pixel[3])) * (F32((palette->getColors())[index].alpha) - F32(pixel[3])));
}


void quantizeRGB(const GBitmap& in, GBitmap* out)
{
   U32 numPixels = 0;
   for (U32 i = 0; i < in.getNumMipLevels(); i++)
      numPixels += (in.getWidth(i) * in.getHeight(i));

   F32* pInputFloatBuffer     = new F32[numPixels * 3];
   quantVector* pInputVectors = new quantVector[numPixels];
   for (int i = 0; i < numPixels; i++) {
      pInputVectors[i].numDim = 3;
      pInputVectors[i].pElem  = &pInputFloatBuffer[i * 3];
      pInputVectors[i].weight = 1.0f;
   }

   F32* pOutputFloatBuffer     = new F32[256 * 3];
   quantVector* pOutputVectors = new quantVector[256];
   for (int i = 0; i < 256; i++) {
      pOutputVectors[i].numDim = 3;
      pOutputVectors[i].pElem  = &pOutputFloatBuffer[i * 3];
      pOutputVectors[i].weight = 0.0f;
   }

   U32 curr = 0;
   for (U32 i = 0; i < in.getNumMipLevels(); i++)
   {
      for (U32 x = 0; x < in.getWidth(i); x++)
      {
         for (U32 y = 0; y < in.getHeight(i); y++)
         {
            const U8* pPixel = in.getAddress(x, y, i);
            pInputVectors[curr].pElem[0] = F32(pPixel[0]) / 255.0f;
            pInputVectors[curr].pElem[1] = F32(pPixel[1]) / 255.0f;
            pInputVectors[curr].pElem[2] = F32(pPixel[2]) / 255.0f;
            curr++;
         }
      }
   }
   AssertFatal(curr == numPixels, "Oh, crap.");
   
   // And quantize...
   //
   S32 numDesiredColors = 256;
   quantizeVectors(pInputVectors,  numPixels,
                   pOutputVectors, numDesiredColors);

   GPalette* pPalette = new GPalette;
   pPalette->setPaletteType(GPalette::RGB);
   dMemset(pPalette->getColors(), 0, 256 * sizeof(ColorI));
   for (U32 i = 0; i < numDesiredColors; i++)
   {
      (pPalette->getColors())[i].red   = U8((pOutputVectors[i].pElem[0] * 255.0f) + 0.5f);
      (pPalette->getColors())[i].green = U8((pOutputVectors[i].pElem[1] * 255.0f) + 0.5f);
      (pPalette->getColors())[i].blue  = U8((pOutputVectors[i].pElem[2] * 255.0f) + 0.5f);
      (pPalette->getColors())[i].alpha = 255;
   }
   
   out->allocateBitmap(in.getWidth(), in.getHeight(), in.getNumMipLevels() != 1, GBitmap::Palettized);
   out->pPalette = pPalette;
   
   for (U32 i = 0; i < in.getNumMipLevels(); i++)
   {
      for (U32 x = 0; x < in.getWidth(i); x++)
      {
         for (U32 y = 0; y < in.getHeight(i); y++)
         {
            const U8* pPixel = in.getAddress(x, y, i);
            U8* pOutputPixel = out->getAddress(x, y, i);

            F32 minDist = calcDistRGB(pPalette, pPixel, 0);
            U8 minIndex = 0;
            for (U32 i = 1; i < numDesiredColors; i++)
            {
               F32 newDist = calcDistRGB(pPalette, pPixel, i);
               if (newDist < minDist)
               {
                  minDist = newDist;
                  minIndex = i;
               }
            }
            *pOutputPixel = minIndex;
         }
      }
   }

   delete [] pOutputVectors;
   delete [] pOutputFloatBuffer;
   delete [] pInputVectors;
   delete [] pInputFloatBuffer;
}

void quantizeRGBA(const GBitmap& in, GBitmap* out)
{
   U32 numPixels = 0;
   for (U32 i = 0; i < in.getNumMipLevels(); i++)
      numPixels += (in.getWidth(i) * in.getHeight(i));

   F32* pInputFloatBuffer     = new F32[numPixels * 4];
   quantVector* pInputVectors = new quantVector[numPixels];
   for (int i = 0; i < numPixels; i++) {
      pInputVectors[i].numDim = 4;
      pInputVectors[i].pElem  = &pInputFloatBuffer[i * 4];
      pInputVectors[i].weight = 1.0f;
   }

   F32* pOutputFloatBuffer     = new F32[256 * 4];
   quantVector* pOutputVectors = new quantVector[256];
   for (int i = 0; i < 256; i++) {
      pOutputVectors[i].numDim = 4;
      pOutputVectors[i].pElem  = &pOutputFloatBuffer[i * 4];
      pOutputVectors[i].weight = 0.0f;
   }

   U32 curr = 0;
   for (U32 i = 0; i < in.getNumMipLevels(); i++)
   {
      for (U32 x = 0; x < in.getWidth(i); x++)
      {
         for (U32 y = 0; y < in.getHeight(i); y++)
         {
            const U8* pPixel = in.getAddress(x, y, i);
            pInputVectors[curr].pElem[0] = F32(pPixel[0]) / 255.0f;
            pInputVectors[curr].pElem[1] = F32(pPixel[1]) / 255.0f;
            pInputVectors[curr].pElem[2] = F32(pPixel[2]) / 255.0f;
            pInputVectors[curr].pElem[3] = F32(pPixel[3]) / 255.0f;
            curr++;
         }
      }
   }
   AssertFatal(curr == numPixels, "Oh, crap.");
   
   // And quantize...
   //
   S32 numDesiredColors = 256;
   quantizeVectors(pInputVectors,  numPixels,
                   pOutputVectors, numDesiredColors);

   GPalette* pPalette = new GPalette;
   dMemset(pPalette->getColors(), 0, 256 * sizeof(ColorI));
   pPalette->setPaletteType(GPalette::RGBA);
   for (U32 i = 0; i < numDesiredColors; i++)
   {
      (pPalette->getColors())[i].red   = U8((pOutputVectors[i].pElem[0] * 255.0f) + 0.5f);
      (pPalette->getColors())[i].green = U8((pOutputVectors[i].pElem[1] * 255.0f) + 0.5f);
      (pPalette->getColors())[i].blue  = U8((pOutputVectors[i].pElem[2] * 255.0f) + 0.5f);
      (pPalette->getColors())[i].alpha = U8((pOutputVectors[i].pElem[3] * 255.0f) + 0.5f);
   }
   
   out->allocateBitmap(in.getWidth(), in.getHeight(), in.getNumMipLevels() != 1, GBitmap::Palettized);
   out->pPalette = pPalette;
   
   for (U32 i = 0; i < in.getNumMipLevels(); i++)
   {
      for (U32 x = 0; x < in.getWidth(i); x++)
      {
         for (U32 y = 0; y < in.getHeight(i); y++)
         {
            const U8* pPixel = in.getAddress(x, y, i);
            U8* pOutputPixel = out->getAddress(x, y, i);

            F32 minDist = calcDistRGBA(pPalette, pPixel, 0);
            U8 minIndex = 0;
            for (U32 i = 1; i < numDesiredColors; i++)
            {
               F32 newDist = calcDistRGBA(pPalette, pPixel, i);
               if (newDist < minDist)
               {
                  minDist = newDist;
                  minIndex = i;
               }
            }
            *pOutputPixel = minIndex;
         }
      }
   }

   delete [] pOutputVectors;
   delete [] pOutputFloatBuffer;
   delete [] pInputVectors;
   delete [] pInputFloatBuffer;
}

void GameReactivate()
{

}

void GameDeactivate( bool )
{

}

