#pragma once
#include "OpenSpatialServiceController.h"
class ExampleDelegate : public OpenSpatialDelegate
{
public:
	ExampleDelegate();
	~ExampleDelegate();
	virtual void pointerEventFired(PointerEvent event);
	virtual void gestureEventFired(GestureEvent event);
	virtual void pose6DEventFired(Pose6DEvent event);
	virtual void buttonEventFired(ButtonEvent event);
};

