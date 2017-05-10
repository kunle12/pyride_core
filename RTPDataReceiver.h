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

#ifdef WIN32
#include <winbase.h>
#include <process.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#endif
#include "PyRideCommon.h"

namespace pyride_remote {
class RTPDataReceiver {
public:
  RTPDataReceiver();
  void init( int port, bool isVideoStream );
  void fini();

  int grabData( unsigned char ** dataBuffer, bool & dataSizeChanged );

  void setStreamSource( const char * host, short controlPort, short dataPort );

  void firewallPunching();

private:
  void * streamSession_;

  SOCKET_T controlSocket_;
  SOCKET_T dataSocket_;

  struct sockaddr_in  cSourceAddr_;
  struct sockaddr_in  dSourceAddr_;

  unsigned int receiveTimestamp_;
  unsigned int lastSeqNum_;
  unsigned char * dataBuffer_;
  int dataBufferSize_;
  int existDataSize_;
};
}
#endif // RTP_DATA_RECEIVER_H
