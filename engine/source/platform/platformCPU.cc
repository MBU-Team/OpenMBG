//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

// $Id: platformCPU.cc,v 2.1 2004/04/21 04:27:32 timg Exp $

#include "platform/platform.h"
#include "core/stringTable.h"

enum CPUFlags
{
   BIT_FPU     = BIT(0),
   BIT_RDTSC   = BIT(4),
   BIT_MMX     = BIT(23),
   BIT_SSE     = BIT(25),  
   BIT_3DNOW   = BIT(31),  
};

// fill the specified structure with information obtained from asm code
void SetProcessorInfo(Platform::SystemInfo_struct::Processor& pInfo, 
   char* vendor, U32 processor, U32 properties)
{
   Platform::SystemInfo.processor.properties |= (properties & BIT_FPU)   ? CPU_PROP_FPU : 0;
   Platform::SystemInfo.processor.properties |= (properties & BIT_RDTSC) ? CPU_PROP_RDTSC : 0;
   Platform::SystemInfo.processor.properties |= (properties & BIT_MMX)   ? CPU_PROP_MMX : 0;

   if (dStricmp(vendor, "GenuineIntel") == 0)
   {
      pInfo.properties |= (properties & BIT_SSE) ? CPU_PROP_SSE : 0;
      pInfo.type = CPU_Intel_Unknown;
      // switch on processor family code
      switch ((processor >> 8) & 0x0f) 
      {
         case 4:
            pInfo.type = CPU_Intel_486;
            pInfo.name = StringTable->insert("Intel 486 class");
            break;

            // Pentium Family
         case 5:
            // switch on processor model code
            switch ((processor >> 4) & 0xf)
            {
               case 1: 
               case 2: 
               case 3: 
                  pInfo.type = CPU_Intel_Pentium;
                  pInfo.name = StringTable->insert("Intel Pentium");
                  break;
               case 4:
                  pInfo.type = CPU_Intel_PentiumMMX;
                  pInfo.name = StringTable->insert("Intel Pentium MMX");
                  break;
               default:
                  pInfo.type = CPU_Intel_Pentium;
                  pInfo.name = StringTable->insert( "Intel (unknown, Pentium family)" );
                  break;
            }
            break;

            // Pentium Pro/II/II family
         case 6:
            // switch on processor model code
            switch ((processor >> 4) & 0xf) 
            {
               case 1:
                  pInfo.type = CPU_Intel_PentiumPro;
                  pInfo.name = StringTable->insert("Intel Pentium Pro");
                  break;
               case 3:
               case 5:
                  pInfo.type = CPU_Intel_PentiumII;
                  pInfo.name = StringTable->insert("Intel Pentium II");
                  break;
               case 6: 
                  pInfo.type = CPU_Intel_PentiumCeleron;
                  pInfo.name = StringTable->insert("Intel Pentium Celeron");
                  break;
               case 7: 
               case 8:
               case 10:
               case 11:
                  pInfo.type = CPU_Intel_PentiumIII;
                  pInfo.name = StringTable->insert("Intel Pentium III");
                  break;
               default:
                  pInfo.type = CPU_Intel_PentiumPro;
                  pInfo.name = StringTable->insert( "Intel (unknown, Pentium Pro/II/III family)" );
                  break;
            }
            break;

            // Pentium4 Family
         case 0xf:
            pInfo.type = CPU_Intel_Pentium4;
            pInfo.name = StringTable->insert( "Intel Pentium 4" );
            break;

         default:
            pInfo.type = CPU_Intel_Unknown;
            pInfo.name = StringTable->insert( "Intel (unknown)" );
            break;
      }
   }
   //--------------------------------------
   else
      if (dStricmp(vendor, "AuthenticAMD") == 0)
      {
         // AthlonXP processors support SSE
         pInfo.properties |= (properties & BIT_SSE) ? CPU_PROP_SSE : 0;
         pInfo.properties |= (properties & BIT_3DNOW) ? CPU_PROP_3DNOW : 0;
         // switch on processor family code
         switch ((processor >> 8) & 0xf)
         {
            // K6 Family
            case 5: 
               // switch on processor model code
               switch ((processor >> 4) & 0xf)
               {
                  case 0:
                  case 1:
                  case 2:
                  case 3:
                     pInfo.type = CPU_AMD_K6_3;
                     pInfo.name = StringTable->insert("AMD K5");
                     break;
                  case 4:
                  case 5:
                  case 6:
                  case 7:
                     pInfo.type = CPU_AMD_K6;
                     pInfo.name = StringTable->insert("AMD K6");
                     break;
                  case 8:
                     pInfo.type = CPU_AMD_K6_2;
                     pInfo.name = StringTable->insert("AMD K6-2");
                     break;
                  case 9:
                  case 10:
                  case 11:
                  case 12:
                  case 13:
                  case 14:
                  case 15:
                     pInfo.type = CPU_AMD_K6_3;
                     pInfo.name = StringTable->insert("AMD K6-3");
                     break;
               }
               break;

               // Athlon Family
            case 6: 
               pInfo.type = CPU_AMD_Athlon;
               pInfo.name = StringTable->insert("AMD Athlon");
               break;

            default:
               pInfo.type = CPU_AMD_Unknown;
               pInfo.name = StringTable->insert("AMD (unknown)");
               break;
         }
      }
   //--------------------------------------
      else
         if (dStricmp(vendor, "CyrixInstead") == 0)
         {
            switch (processor)
            {
               case 0x520:
                  pInfo.type = CPU_Cyrix_6x86;
                  pInfo.name = StringTable->insert("Cyrix 6x86");
                  break;
               case 0x440:
                  pInfo.type = CPU_Cyrix_MediaGX;
                  pInfo.name = StringTable->insert("Cyrix Media GX");
                  break;
               case 0x600:
                  pInfo.type = CPU_Cyrix_6x86MX;
                  pInfo.name = StringTable->insert("Cyrix 6x86mx/MII");
                  break;
               case 0x540:
                  pInfo.type = CPU_Cyrix_GXm;
                  pInfo.name = StringTable->insert("Cyrix GXm");
                  break;
               default:
                  pInfo.type = CPU_Cyrix_Unknown;
                  pInfo.name = StringTable->insert("Cyrix (unknown)");
                  break;
            }
         }
}
