/*
 *  RTPDataReceiver.cpp
 *  PyRIDE
 *
 *  Created by Xun Wang on 11/09/09.
 *  Copyright 2009 Galaxy Network. All rights reserved.
 *
 */
#include <ccrtp/rtp.h>
#include <errno.h>

#include "RTPDataReceiver.h"

namespace pyride_remote {
using namespace std;
using namespace ost;

static const int VideoStreamID = 101;
static const char punchData[] = "xd[eaddc";

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

    int optval = 1228800;
    setsockopt( recvSock, SOL_SOCKET, SO_RCVBUF, (char *)&optval, sizeof( int ) );
    ((RTPSession *)streamSession_)->setSessionBandwidth( 8000000 );
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
    aduSize = (int)adu->getSize();
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
      printf( "RTPReceiver: loss of more than 10 packets\n" );
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

void RTPDataReceiver::setStreamSource( const char * host, short controlPort, short dataPort )
{
  struct hostent * hostInfo = gethostbyname( host );
  if (!hostInfo) { // check for IP
#ifdef WIN32
    unsigned int hostIP;
    hostIP = inet_addr( host );
#else
    unsigned long hostIP;
    if (inet_pton( AF_INET, host, &hostIP ) != 1) {
      printf( "RTPDataReceiver::setStreamSource: invalid hostname or address %s\n",
                host );
      return;
    }
#endif
    hostInfo = gethostbyaddr( (char *)&hostIP, sizeof( hostIP ), AF_INET );
    if (hostInfo == NULL) {
      printf( "RTPDataReceiver::setStreamSource: unable to get host info for %s\n", host );
      return;
    }
  }
  // finally put into sockaddr_in structure
  cSourceAddr_.sin_family = hostInfo->h_addrtype;
  memcpy( (char *)&cSourceAddr_.sin_addr, hostInfo->h_addr, hostInfo->h_length );
  cSourceAddr_.sin_port = controlPort;

  dSourceAddr_.sin_family = hostInfo->h_addrtype;
  memcpy( (char *)&dSourceAddr_.sin_addr, hostInfo->h_addr, hostInfo->h_length );
  dSourceAddr_.sin_port = dataPort;
  //DEBUG_MSG( "got control/data port %d/%d\n", ntohs( controlPort ), ntohs( dataPort ) );
}

void RTPDataReceiver::firewallPunching()
{
  //DEBUG_MSG( "Punching time (%d/%d)\n", ntohs(cSourceAddr_.sin_port), ntohs(dSourceAddr_.sin_port) );
  if (sendto( dataSocket_, punchData, 8, 0, (struct sockaddr *)&dSourceAddr_, sizeof( dSourceAddr_ ) ) == -1) {
    printf( "failed to punch reason %s!\n", strerror(errno) );
  }
  sendto( controlSocket_, punchData, 8, 0, (struct sockaddr *)&cSourceAddr_, sizeof( cSourceAddr_ ) );
}

}
