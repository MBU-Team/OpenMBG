#ifndef __DTSENDIAN_H
#define __DTSENDIAN_H

// Big endian support.  Note that one should not hold onto the 
// returned reference in the FIX_ENDIAN methods.  Return reference
// to a static variable so that one can use & operator on return value.

namespace DTS
{

   inline bool isLittleEndian()
   {
      int test = 1;
      char * t0 = (char*)&test;
      ++*t0;
      return (test==2);
   }
   inline void FIX_ENDIAN2(void * val)
   {
      char * ptr = (char*)val;
      char tmp = ptr[0];
      ptr[0] = ptr[1];
      ptr[1] = tmp;
   }

   inline void FIX_ENDIAN4(void * val)
   {
      char * ptr = (char*)val;
      char tmp = ptr[0];
      ptr[0] = ptr[3];
      ptr[3] = tmp;
      tmp = ptr[1];
      ptr[1] = ptr[2];
      ptr[2] = tmp;
   }

   inline short & FIX_ENDIAN(short val)
   {
      if (!isLittleEndian())
         FIX_ENDIAN2(&val);
      static short out;
      out = val;
      return out;
   }

   inline unsigned short & FIX_ENDIAN(unsigned short val)
   {
      return (unsigned short&)FIX_ENDIAN(short(val));
   }

   inline int & FIX_ENDIAN(int val)
   {
      if (!isLittleEndian())
         FIX_ENDIAN4(&val);
      static int out;
      out = val;
      return out;
   }

   inline unsigned int & FIX_ENDIAN(unsigned int val)
   {
      return (unsigned int&)FIX_ENDIAN( int(val));
   }

   inline float & FIX_ENDIAN(float val)
   {
      if (!isLittleEndian())
         FIX_ENDIAN4(&val);
      static float out;
      out = val;
      return out;
   }

}
#endif