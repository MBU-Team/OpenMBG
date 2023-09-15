//--------------------------------------------
// vorbisStreamSource.h
// header for streaming audio source for Ogg Vorbis
//
// Kurtis Seebaldt
//--------------------------------------------

#ifndef _VORBISSTREAMSOURCE_H_
#define _VORBISSTREAMSOURCE_H_

#ifndef _AUDIOSTREAMSOURCE_H_
#include "audio/audioStreamSource.h"
#endif

#include "audio/vorbisStream.h"

class VorbisStreamSource: public AudioStreamSource
{
	public:
		VorbisStreamSource(const char *filename);
		virtual ~VorbisStreamSource();

		virtual bool initStream();
		virtual bool updateBuffers();
		virtual void freeStream();

	private:
		ALuint				    mBufferList[NUMBUFFERS];
		S32						mNumBuffers;
		S32						mBufferSize;
		Stream				   *stream;

		bool					bReady;
		bool					bFinished;

		ALenum  format;
		ALsizei size;
		ALsizei freq;

		ALuint			DataSize;
		ALuint			DataLeft;
		ALuint			buffersinqueue;

		bool			bBuffersAllocated;
		bool			bVorbisFileInitialized;

		int current_section;
		OggVorbisFile vf;

		void clear();
		long oggRead(char *buffer,int length, int bigendianp,int *bitstream);
		void resetStream();
      void setNewFile(const char * file);
};

#endif // _VORBISSTREAMSOURCE_H_
