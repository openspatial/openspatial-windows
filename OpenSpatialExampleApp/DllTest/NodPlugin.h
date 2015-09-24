/* Copyright 2015 Nod Labs */

// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the NODPLUGIN_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// NODPLUGIN_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#pragma once

#ifdef _WIN32
#ifdef NODPLUGIN_EXPORTS
#define NODPLUGIN_API __declspec(dllexport)
#else
#define NODPLUGIN_API __declspec(dllimport)
#endif
#else
#define NODPLUGIN_API
#endif

#include "Settings.h"

const static int NOD_NONE = -1;

enum Modality {
    ButtonMode = 1,
    AccelMode = 2,
    EulerMode = 3,
    GameControlMode = 4,
    GestureMode = 5,
    PointerMode = 6,
    SliderMode = 7,
    TranslationMode = 9,
    GyroMode = 10
};

enum EventType {
    Button,
    Accelerometer,
    EulerAngles,
    AnalogData,
    Gestures,
    Pointer,
    Slider,
    DataMode,
    Translation,
    Gyroscope,
    DeviceInfo,
    ServiceReady,
    NONE_T = -1
};

enum UpDown {
    NONE_UD = -1,
    UP = 0,
    DOWN = 1
};

enum GestureType {
    NONE_G = -1,
    SWIPE_DOWN = 0,
    SWIPE_LEFT = 1,
    SWIPE_RIGHT = 2,
    SWIPE_UP = 3,
    CW = 4,
    CCW = 5,
};

extern "C" {

    struct NodUniqueID
    {
        char byte0;
        char byte1;
        char byte2;
    };

    struct FirmwareVersion
    {
        int major = NOD_NONE;
        int minor = NOD_NONE;
        int subminor = NOD_NONE;
    };

    struct NodEvent {
        EventType type = NONE_T;
        int x = NOD_NONE;
        int y = NOD_NONE;
        int z = NOD_NONE;
        float xf = NOD_NONE;
        float yf = NOD_NONE;
        float zf = NOD_NONE;
        int trigger = NOD_NONE;
        float roll = NOD_NONE;
        float pitch = NOD_NONE;
        float yaw = NOD_NONE;
        float buttonID = NOD_NONE;
        UpDown buttonState = NONE_UD;
        int batteryPercent = NOD_NONE;
        GestureType gesture = NONE_G;
        FirmwareVersion firmwareVersion;
    };

    NODPLUGIN_API bool NodInitialize(void(*evFiredFn)(NodEvent));
    NODPLUGIN_API bool NodShutdown(void);
    NODPLUGIN_API bool NodRefresh(void);

    NODPLUGIN_API int  NodNumRings(void);
    const NODPLUGIN_API char* NodGetRingName(int ringID);

    NODPLUGIN_API bool NodSubscribe(Modality mode, const char* deviceName);
    NODPLUGIN_API bool NodUnsubscribe(Modality mode, const char* deviceName);

    NODPLUGIN_API bool NodRequestDeviceInfo(const char* deviceName);
    NODPLUGIN_API bool NodChangeSetting(const char* deviceName, Settings setting, int args[], int numArgs);
} // end extern C