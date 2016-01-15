// DllTest.cpp : Defines the entry point for the console application.
//
#include "stdio.h"
#include "NodPlugin.h"
#include "windows.h"
#include <string>

#pragma comment(lib, "OpenSpatialDll.lib")

void eventFired(NodEvent ev, void* ctx)
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
        printf("\nEuler Angle Fired, roll: %f, pitch: %f, yaw: %f from:%s with context: %d",
            ev.roll, ev.pitch, ev.yaw, ev.sender, (int) ctx);
    }
}

int main()
{
    int context = 293;
    NodInitialize(eventFired, (void*) context);
    while (true)
    {
        Sleep(10000);
    }
}

