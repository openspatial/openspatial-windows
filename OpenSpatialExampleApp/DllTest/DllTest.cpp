// DllTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "NodPlugin.h"
#include "windows.h"

#pragma comment(lib, "OpenSpatialDll.lib")

void eventFired(NodEvent ev)
{
    if (ev.type == EventType::ServiceReady)
    {
        if (NodNumRings() > 0)
        {
            NodSubscribe(Modality::EulerMode, NodGetRingName(0));
        }
    }
    else if (ev.type == EventType::EulerAngles)
    {
        printf("\nEuler Angle Fired, roll: %f, pitch: %f, yaw: %f", ev.roll, ev.pitch, ev.yaw);
    }
}

int main()
{
    NodInitialize(eventFired);
    while (true)
    {
        Sleep(10000);
    }
}

