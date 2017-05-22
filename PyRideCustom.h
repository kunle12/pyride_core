/*
 *  PyRideCustom.h
 *  PyRIDE
 *
 *  Created by Xun Wang on 10/06/10.
 *  Copyright 2010 Galaxy Network. All rights reserved.
 *
 */

// This file contains list of custom command and status definitions
// that extends the current communication between robots and 
// remote clients.
#ifndef PyRideCustom_h_DEFINED
#define PyRideCustom_h_DEFINED

typedef enum {
  LEARN_BACKGROUND = 0x0,
  LEARN_MOVEMENT,
  LEARN_POSE,
  LEARN_OBJECT,
  LEARN_STRATEGY,
  LEARN_COMPLETE,
  REPLAY,
  CANCEL_LEARNED,
  FOLLOW_OBJECT,
  LOOK_AT,
  SPEAK,
  HEAD_MOVE_TO,
  BODY_MOVE_TO,
  SAVE_PATH,
  MOVE_IN_PATH,
  UPDATE_HEAD_POSE,
  UPDATE_BODY_POSE,
  EXCLUSIVE_CTRL_REQUEST,
  EXCLUSIVE_CTRL_RELEASE,
  EXCLUSIVE_CTRL_REJECT,
  UPDATE_AUDIO_SETTINGS,
  AUDIO_FEEDBACK_ON,
  VIDEO_FEEDBACK_ON
} PyRideExtendedCommand;

static const int NonExclusiveExtendedCommands[] = { HEAD_MOVE_TO, SPEAK,
  LEARN_OBJECT, UPDATE_AUDIO_SETTINGS, AUDIO_FEEDBACK_ON };
static const int NonExcmdSize = sizeof( NonExclusiveExtendedCommands ) / sizeof( NonExclusiveExtendedCommands[0] );

typedef enum {
  IDLE               = 0x0,
  LEARNING,
  LEARNING_COMPLETE,
  FAULTY_HARDWARE,
  CUSTOM_STATE,
  EXCLUSIVE_CONTROL,
  NORMAL_CONTROL,
  EXCLUSIVE_CONTROL_OVERRIDE,
  AUDIO_SETTINGS_UPDATE
} RobotOperationalState;

#endif // PyRideCustom_h_DEFINED
