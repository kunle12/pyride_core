/*
 *  AudioDataReceiver.cpp
 *  PyRIDE
 *
 *  Created by Xun Wang on 19/04/2018
 *
 */

#include <sys/time.h>
#include <audio_common_msgs/AudioData.h>

#include "AudioDataReceiver.h"

namespace pyride_remote {

using namespace ros;

AudioDataReceiver::AudioDataReceiver( int port, int samplerate, int framesize, int packetbytes ) :
    maxCachedDataSize_( 0 ),
    samplerate_( samplerate ),
    framesize_( framesize ),
    packetbytes_( packetbytes ),
    dataStream_( NULL ),
    celtMode_( NULL ),
    audioDecoder_( NULL )
{
  celtMode_ = celt_mode_create( samplerate, framesize, NULL );

  if (!celtMode_) {
    printf( "Unable to create encoding mode.\n" );
    return;
  }

  maxCachedDataSize_ = 128 * framesize;

  audioDecoder_ = celt_decoder_create_custom( celtMode_, 1, NULL );

  dataStream_ = new RTPDataReceiver();
  dataStream_->init( port, true );
}

AudioDataReceiver::~AudioDataReceiver()
{
  if (audioDecoder_) {
    celt_decoder_destroy( audioDecoder_ );
    audioDecoder_ = NULL;
  }
  if (celtMode_) {
    celt_mode_destroy( celtMode_ );
    celtMode_ = NULL;
  }

  if (dataStream_) {
    delete dataStream_;
    dataStream_ = NULL;
  }
}

int AudioDataReceiver::grabAudioStreamData( short * audioData )
{
  if (!dataStream_ || maxCachedDataSize_ == 0 || audioData == NULL)
    return 0;

  unsigned char * rawData = NULL;
  unsigned char * data = NULL;
  int dataSize = 0, rawDataSize = 0;
  bool dataSizeChanged = false;
  int decodedSize = 0;

  int gcount = 0;

  do {
    rawDataSize = dataStream_->grabData( &rawData, dataSizeChanged );

    data = rawData;
    dataSize = rawDataSize;
    gcount++;
    usleep( 1000 ); // 1ms
  } while (dataSize == 0 && gcount < 10);


  if (dataSize == 0 || dataSize % packetbytes_ != 0) {
    return 0;
  }

  int audioFrames = dataSize / packetbytes_;

  //DEBUG_MSG("Got audio frames %d.\n", audioFrames );

  for (int i = 0;i < audioFrames;i++) {
    celt_decode( audioDecoder_, data+i*packetbytes_, packetbytes_,
        audioData + i * framesize_, framesize_ );
  }
  decodedSize = audioFrames * framesize_ * sizeof( short );
  return decodedSize;
}

} // namespace pyride
