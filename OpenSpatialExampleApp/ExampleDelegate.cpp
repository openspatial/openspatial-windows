#include "stdafx.h"
#include "ExampleDelegate.h"


ExampleDelegate::ExampleDelegate()
{
}


ExampleDelegate::~ExampleDelegate()
{
}

void ExampleDelegate::buttonEventFired(ButtonEvent event)
{
	printf("\nButton Event Fired. ButtonEventType: %d from id: %d", event.buttonEventType, event.sender);
}

void ExampleDelegate::pointerEventFired(PointerEvent event)
{
	printf("\nPointer Event Fired. Pointer X: %d, Pointer Y: %d from id: %d", event.x, event.y, event.sender);
}

void ExampleDelegate::gestureEventFired(GestureEvent event)
{
	printf("\nGesture Event Fired. Gesture Type: %d from id: %d", event.gestureType, event.sender);
}

void ExampleDelegate::pose6DEventFired(Pose6DEvent event)
{
	printf("\nPose6D Event Fired. Yaw: %f, Pitch: %f, Roll %f from id: %d", event.yaw, event.pitch, event.roll, event.sender);
}