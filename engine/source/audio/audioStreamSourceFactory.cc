//--------------------------------------
// audioStreamSource.cc
// implementation of streaming audio source
//
// Kurtis Seebaldt
//--------------------------------------

#include "audio/audioStreamSourceFactory.h"

#include "audio/wavStreamSource.h"
#ifndef NO_OGGVORBIS
#include "audio/vorbisStreamSource.h"
#endif

AudioStreamSource* AudioStreamSourceFactory::getNewInstance(const char *filename)  { 
	S32 len = dStrlen(filename);

	// Check filename extension and guess filetype from that

#ifndef NO_OGGVORBIS
	if(len > 3 && !dStricmp(filename + len - 4, ".wav"))
		return new WavStreamSource(filename);
	else if(len > 3 && !dStricmp(filename + len - 4, ".ogg"))
		return new VorbisStreamSource(filename);
#endif
	return NULL;
}
