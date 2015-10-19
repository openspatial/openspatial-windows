/* Copyright 2015 Nod Labs */

// This file is a list of generic maximal settings that can be enacted on
// any device

#pragma once

enum settingType{
    OCONTROL,
    NCONTROL
};

enum Settings
{
    OCONTROL_GET_PARAMETER,
    OCONTROL_SET_PARAMETER,
    OCONTROL_GET_ID,
    OCONTROL_GET_PARAMETER_RANGE,
    OCONTROL_ENABLE_DATA,
    OCONTROL_DISABLE_DATA,
    O_TO_N,
    NOD_CONTROL_SCREEN_RESOLUTION,
    NOD_CONTROL_INPUT_QUEUE_DEPTH,
    NOD_CONTROL_CALIBRATE_ACCEL,
    NOD_CONTROL_RECALIBRATE,
    NOD_CONTROL_SHUTDOWN,
    NOD_CONTROL_FLIP_Y,
    NOD_CONTROL_FLIP_CW_CCW,
    NOD_CONTROL_TAP_TIME,
    NOD_CONTROL_DOUBLE_TAP_TIME,
    NOD_CONTROL_READ_BATTERY,
    NOD_CONTROL_TOUCH_SENSE,
    NOD_CONTROL_MODE_CHANGE,
    NOD_CONTROL_READ_SIGNATURE,
    NOD_CONTROL_TOUCH_DEBUG,
    NOD_CONTROL_SET_LEDS,
    NOD_CONTROL_FIND_TAG,
    NOD_CONTROL_READ_MEMORY,
    NOD_CONTROL_SINGLE_CALIBRATE,
    NOD_CONTROL_FORGET_CLIENT,
    NOD_CONTROL_FORGET_ALL_CLIENTS,
    NOD_CONTROL_REPORT_MODE,
    NOD_CONTROL_COMPASS_CALIBRATED,
    NOD_CONTROL_READ_SENSOR_BIASES
};

enum Mode
{
    POINTER = 0x00,
    GPAD_L = 0x01,
    GPAD_R = 0x02,
    FREE_POINTER = 0x03
};

enum OTAStatusEnum {
    OTA_NOT_STARTED = 0x00,
    OTA_DOWNLOADING = 0x01,
    OTA_BCMSPEC = 0x02,
    OTA_BCMAPP = 0x03,
    OTA_LOADERSPEC = 0x04,
    OTA_LOADER = 0x05,
    OTA_STMSPEC = 0x06,
    OTA_STMAPP = 0x07,
    //INSERT MORE STATUS HERE all status are 0x0X
    OTA_NO_NETWORK = 0xF0,
    OTA_FAILURE = 0xF1,
    //INTERT MORE FAILURES HERE all failures are 0xFX
    OTA_FINISHED = 0xFF,
};