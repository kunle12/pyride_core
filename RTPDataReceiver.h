/*
 *  RTPDataReceiver.h
 *  PyRIDE
 *
 *  Created by Xun Wang on 11/09/09.
 *  Copyright 2009 Galaxy Network. All rights reserved.
 *
 */

#ifndef RTP_DATA_RECEIVER_H
#define RTP_DATA_RECEIVER_H

namespace pyride_remote {
class RTPDataReceiver {
public:
  RTPDataReceiver();
  void init( int port, bool isVideoStream );
  void fini();

  int grabData( unsigned char ** dataBuffer, bool & dataSizeChanged );

private:
  void * streamSession_;
  unsigned int receiveTimestamp_;
  unsigned int lastSeqNum_;
  unsigned char * dataBuffer_;
  int dataBufferSize_;
  int existDataSize_;
};
}
#endif // RTP_DATA_RECEIVER_H
