/*
 *  AudioDataReceiver.h
 *  PyRIDE
 *
 *  Created by Xun Wang on 19/04/2018
 *
 */

#ifndef AUDIO_DATA_RECEIVER_H
#define AUDIO_DATA_RECEIVER_H

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#include <ros/ros.h>

#include <opus/opus.h>

#include "RTPDataReceiver.h"

namespace pyride_remote {

using namespace ros;

class AudioDataReceiver
{
public:
  AudioDataReceiver( int port, int samplerate, int framesize, int packetbytes );
  ~AudioDataReceiver();

  int grabAudioStreamData( short * audioData );

private:
  int samplerate_;
  int framesize_;
  int packetbytes_;

  RTPDataReceiver * dataStream_;

  OpusDecoder * audioDecoder_;
};

} // namespace pyride

#endif /* AUDIO_DATA_RECEIVER_H */
