/*
 *  ConsoleDataProcessor.cpp
 *  PyRIDE
 *
 *  Created by Xun Wang on 14/05/10.
 *  Copyright 2010 GalaxyNetwork. All rights reserved.
 *
 */
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ObjBase.h>
#endif

#include "ConsoleDataProcessor.h"

void initConsoleDataProcessor()
{
  ConsoleDataProcessor::instance()->init();
}

void finiConsoleDataProcessor()
{
  ConsoleDataProcessor::instance()->fini();
}

void processingData()
{
  ConsoleDataProcessor::instance()->processingData();
}

void setRobotCallbacks( void (* addRobotFn)( const char, const int, const RobotInfo *, const VideoSettings *, const AudioSettings *,
                                        const unsigned char *, const int ),
                     void (* removeRobotFn)( const char ),
                     void (* telemetryFn)( const char, const RobotPose *, const FieldObject *, const int ),
                     void (* isTelemetryStreamStartedFn)( bool ),
                     void (* isImageStreamStartedFn)( bool, const char ),
                     void (* onCameraModeUpdateFn)( const char, ImageFormat ),
                     void (* onCameraSwitchFn)( const char, const VideoSettings * ),
                     void (* operationalDataFn)( const char, const int, const unsigned char *, const int ),
                     void (* extCommandRespFn)( const char, const PyRideExtendedCommand,
                                                const unsigned char *, const int ) )
{
  ConsoleDataProcessor::instance()->setRobotDataCallbacks( addRobotFn, removeRobotFn, telemetryFn,
                                                        isTelemetryStreamStartedFn, 
                                                        isImageStreamStartedFn,
                                                        onCameraModeUpdateFn,
                                                        onCameraSwitchFn,
                                                        operationalDataFn,
                                                        extCommandRespFn);
}

void discoverRobots()
{
  ConsoleDataProcessor::instance()->discoverRobots();
}

bool logonToRobot( const char * host, const unsigned char * authCode )
{
  return ConsoleDataProcessor::instance()->logonToRobot( host, authCode );
}

void disconnectRobots()
{
  ConsoleDataProcessor::instance()->disconnectRobots();
}

void startTelemetryStream( const char cID )
{
  ConsoleDataProcessor::instance()->startTelemetryStream( cID );
}

void stopTelemetryStream()
{
  ConsoleDataProcessor::instance()->stopTelemetryStream();
}

void startCameraImageStream( const char cID )
{
  ConsoleDataProcessor::instance()->startCameraImageStream( cID );
}

void stopCameraImageStream( const char cID )
{
  ConsoleDataProcessor::instance()->stopCameraImageStream( cID );
}

void cancelCurrentOperation( const char cID )
{
  ConsoleDataProcessor::instance()->cancelCurrentOperation( cID );
}

void setImageFormat( const char cID, ImageFormat format )
{
  ConsoleDataProcessor::instance()->setImageFormat( cID, format );
}

void switchCamera( const char cID, const char vID )
{
  ConsoleDataProcessor::instance()->switchCamera( cID, vID );
}

void issueExtendedCommand( const char cID, const PyRideExtendedCommand command,
                          const unsigned char * optionalData , const int optionalDataLength )
{
  ConsoleDataProcessor::instance()->issueExtendedCommand( cID, command, optionalData, optionalDataLength );
}

ConsoleDataProcessor * ConsoleDataProcessor::s_pConsoleDataProcessor = NULL;

ConsoleDataProcessor * ConsoleDataProcessor::instance()
{
  if (!s_pConsoleDataProcessor)
    s_pConsoleDataProcessor = new ConsoleDataProcessor();
  return s_pConsoleDataProcessor;
}

ConsoleDataProcessor::ConsoleDataProcessor() :
  RobotDataHandler(),
  pNetComm_( NULL ),
  cmdHandler_( NULL ),
  telemetryRobot_( 0 ),
  addRobotFn_( NULL ),
  removeRobotFn_( NULL ),
  telemetryFn_( NULL ),
  isTelemetryStreamStartedFn_( NULL ),
  isImageStreamStartedFn_( NULL ),
  onCameraModeUpdateFn_( NULL ),
  onCameraSwitchFn_( NULL ),
  operationalDataFn_( NULL ),
  extCommandRespFn_( NULL )
{
#ifdef WIN32
#ifndef NO_WINCOM
  CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );
#endif
  WSAData wsaData;
  int nStart = 0;
  
  if ((nStart = WSAStartup( 0x202, &wsaData )) != 0) {
    DEBUG_MSG( "WebcamManager: winsock 2 DLL initialization failed\n" );
    WSASetLastError( nStart );
  }
#endif
}

ConsoleDataProcessor::~ConsoleDataProcessor()
{
#ifdef WIN32
  WSACleanup();
#ifndef NO_WINCOM
  CoUninitialize();
#endif
#endif
}

void ConsoleDataProcessor::init( PyRideConsoleCommandHandler * cmdHander )
{
  if (!pNetComm_) {
    pNetComm_ = new PyRideNetComm( this );
    pNetComm_->init();
    int myID = 0;
    if (pNetComm_->getIDFromIP( myID ))
      clientID_ = myID;
  }
  cmdHandler_ = cmdHander;
}

void ConsoleDataProcessor::fini()
{
  if (pNetComm_) {
    pNetComm_->fini();
  }
}

void ConsoleDataProcessor::processingData()
{
  if (pNetComm_) {
    pNetComm_->continuousProcessing();
  }
}

void ConsoleDataProcessor::setRobotDataCallbacks( void (* addRobotFn)( const char, const int, const RobotInfo *, const VideoSettings *, const AudioSettings *, const unsigned char *, const int ),
                                                void (* removeRobotFn)( const char ),
                                                void (* telemetryFn)( const char, const RobotPose *, const FieldObject *, const int ),
                                                void (* isTelemetryStreamStartedFn)( bool ),
                                                void (* isImageStreamStartedFn)( bool, const char ),
                                                void (* onCameraModeUpdateFn)( const char, ImageFormat ),
                                                void (* onCameraSwitchFn)( const char, const VideoSettings * ),
                                                void (* operationalDataFn)( const char, const int, const unsigned char *, const int ),
                                                void (* extCommandRespFn)( const char, const PyRideExtendedCommand,
                                                                         const unsigned char *, const int ) )
{
  addRobotFn_ = addRobotFn;
  removeRobotFn_ = removeRobotFn;
  telemetryFn_ = telemetryFn;
  isTelemetryStreamStartedFn_ = isTelemetryStreamStartedFn;
  isImageStreamStartedFn_ = isImageStreamStartedFn;
  onCameraModeUpdateFn_ = onCameraModeUpdateFn;
  onCameraSwitchFn_ = onCameraSwitchFn;
  operationalDataFn_ = operationalDataFn;
  extCommandRespFn_ = extCommandRespFn;
}

void ConsoleDataProcessor::onRobotCreated( const char cID, const int ipAddr, const RobotInfo * rinfo,
                                        const VideoSettings * vsettings, const AudioSettings * asettings,
                                        const unsigned char * optLabel, const int optLabelLength )
{
  if (addRobotFn_) {
    (addRobotFn_)( cID, ipAddr, rinfo, vsettings, asettings, optLabel, optLabelLength );
  }
  if (cmdHandler_) {
    cmdHandler_->onRobotCreated( cID, ipAddr, rinfo, vsettings, asettings, optLabel, optLabelLength );
  }
}

void ConsoleDataProcessor::onRobotDestroyed( const char cID )
{
  if (removeRobotFn_) {
    (removeRobotFn_)( cID );
  }
  if (cmdHandler_) {
    cmdHandler_->onRobotDestroyed( cID );
  }
  if (telemetryRobot_ > 0) {
    telemetryRobot_--;
  }
}

void ConsoleDataProcessor::onRobotTelemetryData( const char cID, const RobotPose * pose, const FieldObject * objects,
                                            const int nofObjs )
{
  if (telemetryFn_) {
    (telemetryFn_)( cID, pose, objects, nofObjs );
  }
  if (cmdHandler_) {
    cmdHandler_->onTelemetryData( cID, pose, objects, nofObjs );
  }
}

void ConsoleDataProcessor::onTelemetryStreamStart( const char cID )
{
  if (telemetryRobot_ == 0) {
    if (isTelemetryStreamStartedFn_) {
      (isTelemetryStreamStartedFn_)( true );
    }
    if (cmdHandler_) {
      cmdHandler_->onTelemetryStreamControl( true );
    }
  }
  telemetryRobot_++;
}

void ConsoleDataProcessor::onTelemetryStreamStop( const char cID )
{
  telemetryRobot_--;
  if (telemetryRobot_ == 0) {
    if (isTelemetryStreamStartedFn_) {
      (isTelemetryStreamStartedFn_)( false );
    }
    if (cmdHandler_) {
      cmdHandler_->onTelemetryStreamControl( false );
    }
  }
}

void ConsoleDataProcessor::onImageStreamStart( const char cID )
{
  if (isImageStreamStartedFn_) {
    (isImageStreamStartedFn_)( true, cID );
  }
  if (cmdHandler_) {
    cmdHandler_->onVideoStreamControl( true, cID );
  }
}

void ConsoleDataProcessor::onImageStreamStop( const char cID )
{
  if (isImageStreamStartedFn_) {
    (isImageStreamStartedFn_)( false, cID );
  }
  if (cmdHandler_) {
    cmdHandler_->onVideoStreamControl( false, cID );
  }
}

void ConsoleDataProcessor::onImageFormatChange( const char cID, ImageFormat format )
{
  if (onCameraModeUpdateFn_) {
    (onCameraModeUpdateFn_)( cID, format );
  }
}

void ConsoleDataProcessor::onVideoSwitchChange( const char cID, const VideoSettings * vsettings )
{
  if (onCameraSwitchFn_) {
    (onCameraSwitchFn_)( cID, vsettings );
  }
  if (cmdHandler_) {
    cmdHandler_->onVideoStreamSwitch( cID, vsettings );
  }
}

void ConsoleDataProcessor::onOperationalData( const char cID, const int status, const unsigned char * optionalData,
                                             const int optionalDataLength )
{
  if (operationalDataFn_) {
    (operationalDataFn_)( cID, status, optionalData, optionalDataLength );
  }
  if (cmdHandler_) {
    cmdHandler_->onOperationalData( cID, status, optionalData, optionalDataLength );
  }
}

void ConsoleDataProcessor::onExtendedCommandResponse( const char cID, const PyRideExtendedCommand command,
                                                     const unsigned char * optionalData,
                                                     const int optionalDataLength )
{
  if (extCommandRespFn_) {
    (extCommandRespFn_)( cID, command, optionalData, optionalDataLength );
  }
  if (cmdHandler_) {
    cmdHandler_->onExtendedCommandResponse( cID, command, optionalData, optionalDataLength );
  }
}

void ConsoleDataProcessor::discoverRobots()
{
  if (pNetComm_) {
    pNetComm_->discoverRobots();
  }
}

bool ConsoleDataProcessor::logonToRobot( const char * host, const unsigned char * authCode )
{
  if (pNetComm_) {
    return pNetComm_->logonToRobot( host, authCode );
  }
  return false;
}

void ConsoleDataProcessor::disconnectRobots()
{
  if (pNetComm_) {
    pNetComm_->disconnectRobots();
  }
  telemetryRobot_ = 0;
}

void ConsoleDataProcessor::startTelemetryStream( const char cID )
{
  if (pNetComm_) {
    pNetComm_->startTelemetryStream( cID );
  }
}

void ConsoleDataProcessor::stopTelemetryStream()
{
  if (pNetComm_) {
    pNetComm_->stopTelemetryStream();
  }
}
void ConsoleDataProcessor::startCameraImageStream( const char cID )
{
  if (pNetComm_) {
    pNetComm_->startCameraImageStream( cID );
  }
}

void ConsoleDataProcessor::stopCameraImageStream( const char cID )
{
  if (pNetComm_) {
    pNetComm_->stopCameraImageStream( cID );
  }
}

void ConsoleDataProcessor::cancelCurrentOperation( const char cID )
{
  if (pNetComm_) {
    pNetComm_->cancelCurrentOperation( cID );
  }
}

void ConsoleDataProcessor::issueExtendedCommand( const char cID, const PyRideExtendedCommand command, 
                                                const unsigned char * optionalData, const int optionalDataLength )
{
  if (pNetComm_) {
    pNetComm_->issueExtendedCommand( cID, command, optionalData, optionalDataLength );
  }
}

void ConsoleDataProcessor::setImageFormat( const char cID, ImageFormat format )
{
  if (pNetComm_) {
    pNetComm_->setImageFormat( cID, format );
  }
}

void ConsoleDataProcessor::switchCamera( const char cID, const char vID )
{
  if (pNetComm_) {
    pNetComm_->switchCamera( cID, vID );
  }
}
