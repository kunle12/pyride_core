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

static int kSupportedAudioSamplingRate[] = {8000, 12000, 16000, 24000, 48000};
static const int kSupportedAudioSamplingRateSize = sizeof( kSupportedAudioSamplingRate ) / sizeof( kSupportedAudioSamplingRate[0] );

AudioDataReceiver::AudioDataReceiver( int port, int samplerate, int framesize, int packetbytes ) :
    samplerate_( samplerate ),
    framesize_( framesize ),
    packetbytes_( packetbytes ),
    dataStream_( NULL ),
    audioDecoder_( NULL )
{
  bool supported = false;
  for (int i = 0; i < kSupportedAudioSamplingRateSize; i++) {
    if (samplerate == kSupportedAudioSamplingRate[i]) {
      supported = true;
      break;
    }
  }

  if (!supported) {
    //ERROR_MSG( "Unsupported audio sampling rate.\n" );
    return;
  }

  int err = 0;

  audioDecoder_ = opus_decoder_create( samplerate, 1, &err );

  if (!audioDecoder_) {
    //ERROR_MSG( "Unable to initialise audio decoder.\n" );
    return;
  }

  dataStream_ = new RTPDataReceiver();
  dataStream_->init( port, true );
}

AudioDataReceiver::~AudioDataReceiver()
{
  if (audioDecoder_) {
    opus_decoder_destroy( audioDecoder_ );
    audioDecoder_ = NULL;
  }

  if (dataStream_) {
    delete dataStream_;
    dataStream_ = NULL;
  }
}

int AudioDataReceiver::grabAudioStreamData( short * audioData )
{
  if (!dataStream_ || audioData == NULL)
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


  if (dataSize == 0) {
    return 0;
  }

  //DEBUG_MSG("Got audio frames %d.\n", audioFrames );

  int frame_size = opus_decode( audioDecoder_, data, dataSize,
      audioData, samplerate_, 0 );

  decodedSize = frame_size;
  return decodedSize;
}

} // namespace pyride
