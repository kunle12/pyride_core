/*
 *  ConsoleDataProcessor.h
 *  Console2
 *
 *  Created by Xun Wang on 14/05/10.
 *  Copyright 2010 GalaxyNetwork. All rights reserved.
 *
 */

#ifndef ConsoleDataProcessor_h_DEFINED
#define ConsoleDataProcessor_h_DEFINED

#include "PyRideNetComm.h"

#ifdef __cplusplus
using namespace pyride;

extern "C" {
  void initConsoleDataProcessor();
  void finiConsoleDataProcessor();
  void processingData();
  void setRobotCallbacks( void (* addRobotFn)( const char, const int, const RobotInfo *,
                                          const VideoSettings * vsettings, const AudioSettings * asettings,
                                          const unsigned char * optLabel, const int optLabelLength ),
                       void (* removeRobotFn)( const char ),
                       void (* telemetryFn)( const char, const RobotPose *, const FieldObject *, const int ),
                       void (* isTelemetryStreamStartedFn)( bool ),
                       void (* isImageStreamStartedFn)( bool, const char ),
                       void (* onCameraModeUpdateFn)( const char, ImageFormat ),
                       void (* onCameraSwitchFn)( const char, const VideoSettings * ),
                       void (* operationalDataFn)( const char, const int, const unsigned char *, const int ),
                       void (* extCommandRespFn)( const char, const PyRideExtendedCommand,
                                                   const unsigned char *, const int ) );
  bool logonToRobot( const char * host, const unsigned char * authCode );
  void discoverRobots();
  void disconnectRobots();
  void startTelemetryStream( const char cID );
  void stopTelemetryStream();
  void startCameraImageStream( const char cID );
  void stopCameraImageStream( const char cID );
  void cancelCurrentOperation( const char cID );
  void setImageFormat( const char cID, ImageFormat format );
  void switchCamera( const char cID, const char vID );
  void issueExtendedCommand( const char cID, const PyRideExtendedCommand command,
                            const unsigned char * optionalData , const int optionalDataLength );
};

//using namespace std;

class PyRideConsoleCommandHandler
{
protected:
  virtual void onRobotCreated( const char cID, const int ipAddr, const RobotInfo * rinfo,
                            const VideoSettings * vsettings, const AudioSettings * asettings,
                            const unsigned char * optLabel,
                            const int optLabelLength ) = 0;

  virtual void onRobotDestroyed( const char cID ) = 0;
  virtual void onTelemetryData( const char cID, const RobotPose * pose, const FieldObject * objects = NULL,
                                const int nofObjs = 0 ) {}
  virtual void onTelemetryStreamControl( bool isStart ) {}
  virtual void onVideoStreamControl( bool isStart, const char cID ) {}
  virtual void onVideoStreamSwitch( const char cID, const VideoSettings * vsettings ) {}
  virtual void onOperationalData( const char cID, const int status,
                                  const unsigned char * optionalData = NULL,
                                  const int optionalDataLength = 0 ) {}
  virtual void onExtendedCommandResponse( const char cID, const PyRideExtendedCommand command,
                                 const unsigned char * optionalData = NULL,
                                 const int optionalDataLength = 0 ) {}

  friend class ConsoleDataProcessor;
};

class ConsoleDataProcessor : public RobotDataHandler
{
public:
  static ConsoleDataProcessor * instance();
  ~ConsoleDataProcessor();

  void init( PyRideConsoleCommandHandler * cmdHandler = NULL );
  void fini();
  void processingData();
  
  bool logonToRobot( const char * host, const unsigned char * authCode );
  void discoverRobots();
  void disconnectRobots();
  void startTelemetryStream( const char cID = 0 );
  void stopTelemetryStream();
  void startCameraImageStream( const char cID );
  void stopCameraImageStream( const char cID );
  void cancelCurrentOperation( const char cID );
  void issueExtendedCommand( const char cID, const PyRideExtendedCommand command,
                            const unsigned char * optionalData = NULL, const int optionalDataLength = 0 );
  void setRobotDataCallbacks( void (* addRobotFn)( const char, const int, const RobotInfo *, const VideoSettings *, const AudioSettings *,
                                               const unsigned char *, const int ),
                            void (* removeRobotFn)( const char ),
                            void (* telemetryFn)( const char, const RobotPose *, const FieldObject *, const int ),
                            void (* isTelemetryStreamStartedFn)( bool ),
                            void (* isImageStreamStartedFn)( bool, const char ),
                            void (* onCameraModeUpdateFn)( const char, ImageFormat ),
                            void (* onCameraSwitchFn)( const char, const VideoSettings * ),
                            void (* operationalDataFn)( const char, const int, const unsigned char *, const int ),
                            void (* extCommandRespFn)( const char, const PyRideExtendedCommand,
                                                        const unsigned char *, const int ) );
  void setImageFormat( const char cID, ImageFormat format );
  void switchCamera( const char cID, const char vID );

private:
  PyRideNetComm * pNetComm_;
  PyRideConsoleCommandHandler * cmdHandler_;
  int telemetryRobot_;
  //unsigned char * pConvImageData_;
  //int convImageDataSize_;
  
  ConsoleDataProcessor();

  void (* addRobotFn_)( const char, const int, const RobotInfo *, const VideoSettings *,
                     const AudioSettings *, const unsigned char *, const int );
  void (* removeRobotFn_)( const char );
  void (* telemetryFn_)( const char, const RobotPose *, const FieldObject *, const int );
  void (* isTelemetryStreamStartedFn_)( bool );
  void (* isImageStreamStartedFn_)( bool, const char );
  void (* onCameraModeUpdateFn_)( const char, ImageFormat );
  void (* onCameraSwitchFn_)( const char, const VideoSettings * );
  void (* operationalDataFn_)( const char, const int, const unsigned char *, const int );
  void (* extCommandRespFn_)( const char, const PyRideExtendedCommand, const unsigned char *, const int );

  static ConsoleDataProcessor * s_pConsoleDataProcessor;

  void onRobotCreated( const char cID, const int ipAddr, const RobotInfo * rinfo, const VideoSettings * vsettings, const AudioSettings * assetings,
                    const unsigned char * optLabel,
                    const int optLabelLength );
  void onRobotDestroyed( const char cID );
  void onRobotTelemetryData( const char cID, const RobotPose * pose, const FieldObject * objects = NULL,
                          const int nofObjs = 0 );
  void onTelemetryStreamStart( const char cID );
  void onTelemetryStreamStop( const char cID );
  void onImageStreamStart( const char cID );
  void onImageStreamStop( const char cID );
  void onImageFormatChange( const char cID, ImageFormat format );
  void onVideoSwitchChange( const char cID, const VideoSettings * vsettings );
  void onExtendedCommandResponse( const char cID, const PyRideExtendedCommand command,
                                 const unsigned char * optionalData = NULL,
                                 const int optionalDataLength = 0 );
  void onOperationalData( const char cID, const int status,
                         const unsigned char * optionalData = NULL,
                         const int optionalDataLength = 0 );
};
#else
void initConsoleDataProcessor();
void finiConsoleDataProcessor();
void processingData();
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
                                                 const unsigned char *, const int ) );
bool logonToRobot( const char * host, const unsigned char * authCode );
void discoverRobots();
void disconnectRobots();
void startTelemetryStream( const char cID );
void stopTelemetryStream();
void startCameraImageStream( const char cID );
void stopCameraImageStream( const char cID );
void cancelCurrentOperation( const char cID );
void setImageFormat( const char cID, ImageFormat format );
void switchCamera( const char cID, const char vID );
void issueExtendedCommand( const char cID, const PyRideExtendedCommand command,
                          const unsigned char * optionalData , const int optionalDataLength );
#endif // __cplusplus

#endif  // ConsoleDataProcessor_h_DEFINED
