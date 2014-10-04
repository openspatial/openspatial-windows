#include "stdafx.h"
#include "ExampleDelegate.h"

int __cdecl main(int argc, char **argv)
{
	OpenSpatialServiceController* controller;
	controller = new OpenSpatialServiceController;
	ExampleDelegate* del = new ExampleDelegate;
	controller->setDelegate(del);
	BOOL continuer = true;
	while (continuer)
	{
		std::string input;
		printf("\nInput: ");
		std::cin >> input;
		if (strcmp(input.c_str(), "shutdown") == 0)
		{
			printf("\nShutting Nod Down\n");
			controller->shutdown(controller->names.at(0));
		}
		if (strcmp(input.c_str(), "subscribe") == 0)
		{
			int index;
			printf("\nIndex: ");
			std::cin >> index;
			printf("\nSubscribe To: ");
			std::cin >> input;
			if (strcmp(input.c_str(), "pointer") == 0)
			{
				printf("subscribe 2D %s\n", controller->names.at(index).c_str());
				controller->subscribeToPointer(controller->names.at(index));
			}
			if (strcmp(input.c_str(), "gesture") == 0)
			{
				printf("subscribe gesture %s\n", controller->names.at(index).c_str());
				controller->subscribeToGesture(controller->names.at(index));
			}
			if (strcmp(input.c_str(), "button") == 0)
			{
				printf("subscribe button %s\n", controller->names.at(index).c_str());
				controller->subscribeToButton(controller->names.at(index));
			}
			if (strcmp(input.c_str(), "pose6d") == 0)
			{
				printf("subscribe pose6d %s\n", controller->names.at(index).c_str());
				controller->subscribeToPose6D(controller->names.at(index));
			}
		}
		if (strcmp(input.c_str(), "start") == 0)
		{
			printf("starting service\n");
		}
		if (strcmp(input.c_str(), "stop") == 0)
		{
			printf("stopping service\n");
		}
		if (strcmp(input.c_str(), "ttm") == 0)
		{
			printf("switching to ttm\n");
			controller->setMode(controller->names.at(0), MODE_TTM);
		}
		if (strcmp(input.c_str(), "gamepad") == 0)
		{
			printf("switching to gamepad\n");
			controller->setMode(controller->names.at(0), MODE_GAMEPAD);
		}
		if (strcmp(input.c_str(), "pose6d") == 0)
		{
			printf("switching to pose6d\n");
			controller->setMode(controller->names.at(0), MODE_3D);
		}
		if (strcmp(input.c_str(), "bye") == 0)
		{
			printf("exiting");
			delete controller;
			continuer = false;
		}
	}
	return 0;
}



