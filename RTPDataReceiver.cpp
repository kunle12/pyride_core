/*
 *  RTPDataReceiver.cpp
 *  PyRIDE
 *
 *  Created by Xun Wang on 11/09/09.
 *  Copyright 2009 Galaxy Network. All rights reserved.
 *
 */
#include <ccrtp/rtp.h>
#include <cc++/config.h>

#include "RTPDataReceiver.h"
#include "PyRideCommon.h"

namespace pyride_remote {
using namespace std;
using namespace ost;

static const int VideoStreamID = 101;

RTPDataReceiver::RTPDataReceiver() :
  streamSession_( NULL ),
  receiveTimestamp_( 0 ),
  lastSeqNum_( 0 ),
  dataBuffer_( NULL ),
  dataBufferSize_( 0 ),
  existDataSize_( 0 )
{
}

void RTPDataReceiver::init( int port, bool isVideoStream )
{
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons( port );

  streamSession_ = new RTPSession( InetHostAddress( addr.sin_addr ), port );
  // Note receiver may not need to set payload format

  if (isVideoStream) {
    ((RTPSession *)streamSession_)->setPayloadFormat( DynamicPayloadFormat( VideoStreamID, 90000 ) );
    DualRTPUDPIPv4Channel * dataChan = ((RTPSession *)streamSession_)->getDSO();
    SOCKET_T recvSock = dataChan->getRecvSocket();
  
    int optval = 25000;
    setsockopt( recvSock, SOL_SOCKET, SO_RCVBUF, (char *)&optval, sizeof( int ) );
  }
  else {
    ((RTPSession *)streamSession_)->setPayloadFormat( StaticPayloadFormat( sptPCMU ) );
  }
  ((RTPSession *)streamSession_)->startRunning();
}

void RTPDataReceiver::fini()
{
  if (dataBuffer_) {
    delete [] dataBuffer_;
    dataBufferSize_ = 0;
  }
  existDataSize_ = 0;

  if (streamSession_) {
    delete (RTPSession *)streamSession_;
    streamSession_ = NULL;
  }
}

int RTPDataReceiver::grabData( unsigned char ** dataBuffer, bool & dataSizeChanged )
{
  const AppDataUnit * adu = NULL;
  int aduSize = 0;
  
  receiveTimestamp_ = ((RTPSession *)streamSession_)->getFirstTimestamp();
  adu = ((RTPSession *)streamSession_)->getData( receiveTimestamp_ );
  
  // There is no packet available. This may have
  // several reasons:
  // - the thread scheduling granularity does
  //   not match ptime
  // - packet lost
  // - packet delayed
  // Wait another cycle for a packet. The
  // jitter buffer will cope with this variation.
  if (adu && adu->getSize() <= 0) {
    //DEBUG_MSG( "miss 1\n" );
    delete adu;
    adu = NULL;
  }
  
  if (adu) {
    aduSize = adu->getSize();
    if (dataBuffer_ == NULL) { // we have an empty buffer
      dataBufferSize_ = aduSize * 2;
      dataBuffer_ = new unsigned char[dataBufferSize_];
    }
    else if (dataBufferSize_ < aduSize) { // increase our internal buffer
      delete [] dataBuffer_;
      dataBufferSize_ = aduSize * 2;
      dataBuffer_ = new unsigned char[dataBufferSize_];
    }
    // copy data
    int seqNum = adu->getSeqNum();
    if (seqNum - lastSeqNum_ > 10) {
      WARNING_MSG( "RTPReceiver: loss of more than 10 packets\n" );
    }
    lastSeqNum_ = seqNum;

    memcpy( (void *)(dataBuffer_), (void *)adu->getData(), aduSize );

    //DEBUG_MSG( "Got data of size %d\n", aduSize );
    delete adu;
    *dataBuffer = dataBuffer_;
    dataSizeChanged = (aduSize != existDataSize_);
    existDataSize_ = aduSize;
    return aduSize;
  }
  //DEBUG_MSG( "got no data\n" );
  *dataBuffer = NULL;
  return 0;
}
}
