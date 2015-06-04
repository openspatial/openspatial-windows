#include "stdafx.h"
#include "ExampleDelegate.h"

int __cdecl main(int argc, char **argv)
{
	OpenSpatialServiceController* controller;
	controller = new OpenSpatialServiceController;
	ExampleDelegate* del = new ExampleDelegate;
	controller->setDelegate(del);
	BOOL continuer = true;
	//controller->setBufferPackets(true);
	while (!controller->setup)
	{
		Sleep(100);
	}
	while (continuer)
	{
		std::string input;
		printf("\nInput: ");
		std::cin >> input;
		if (strcmp(input.c_str(), "bye") == 0)
		{
			if (controller)
			{
				delete controller;
			}
			continuer = false;
			printf("exiting");
		}
		else
		{
			int index;
			printf("\nIndex: ");
			std::cin >> index;
			if (index >= (int) controller->names.size()) {
			  printf("WARNING: Illegal index %d, assuming 0\n", index);
			  index = 0;
			}
			if (strcmp(input.c_str(), "shutdown") == 0)
			{
				//shutsdown ring at index
				printf("\nShutting Nod Down\n");
				controller->controlService(SHUTDOWN_NOD, controller->names.at(index));
			}
			if (strcmp(input.c_str(), "unsubscribe") == 0)
			{
				printf("\nUnsubscribe To: ");
				std::cin >> input;
				if (strcmp(input.c_str(), "pointer") == 0)
				{
					printf("unsubscribe 2D %s\n", controller->names.at(index));
					controller->controlService(UNSUBSCRIBE_TO_POINTER, controller->names.at(index));
				}
				if (strcmp(input.c_str(), "gesture") == 0)
				{
					printf("unsubscribe gesture %s\n", controller->names.at(index));
					controller->controlService(UNSUBSCRIBE_TO_GESTURE, controller->names.at(index));
				}
				if (strcmp(input.c_str(), "button") == 0)
				{
					printf("unsubscribe button %s\n", controller->names.at(index));
					controller->controlService(UNSUBSCRIBE_TO_BUTTON, controller->names.at(index));
				}
				if (strcmp(input.c_str(), "pose6d") == 0)
				{
					printf("unsubscribe pose6d %s\n", controller->names.at(index));
					controller->controlService(UNSUBSCRIBE_TO_POSE6D, controller->names.at(index));
				}
				if (strcmp(input.c_str(), "game") == 0)
				{
					printf("unsubscribe game %s\n", controller->names.at(index).c_str());
					controller->controlService(UNSUBSCRIBE_TO_GAMECONTROL, controller->names.at(index));
				}
				if (strcmp(input.c_str(), "mot6d") == 0)
				{
					printf("unsubscribe mot6d %s\n", controller->names.at(index));
					controller->controlService(UNSUBSCRIBE_TO_MOT6D, controller->names.at(index));
				}
			}
			else if (strcmp(input.c_str(), "subscribe") == 0)
			{
				printf("\nSubscribe To: ");
				std::cin >> input;
				if (strcmp(input.c_str(), "pointer") == 0)
				{
					printf("subscribe 2D %s\n", controller->names.at(index).c_str());
					controller->controlService(SUBSCRIBE_TO_POINTER, controller->names.at(index));
				}
				if (strcmp(input.c_str(), "gesture") == 0)
				{
					printf("subscribe gesture %s\n", controller->names.at(index).c_str());
					controller->controlService(SUBSCRIBE_TO_GESTURE, controller->names.at(index));
				}
				if (strcmp(input.c_str(), "button") == 0)
				{
					printf("subscribe button %s\n", controller->names.at(index).c_str());
					controller->controlService(SUBSCRIBE_TO_BUTTON, controller->names.at(index));
				}
				if (strcmp(input.c_str(), "pose6d") == 0)
				{
						printf("subscribe pose6d %s\n", controller->names.at(index).c_str());
						controller->controlService(SUBSCRIBE_TO_POSE6D, controller->names.at(index));
				}
				if (strcmp(input.c_str(), "game") == 0)
				{
					printf("subscribe game %s\n", controller->names.at(index).c_str());
					controller->controlService(SUBSCRIBE_TO_GAMECONTROL, controller->names.at(index));
				}
				if (strcmp(input.c_str(), "mot6d") == 0)
				{
					printf("subscribe mot6d %s\n", controller->names.at(index));
					controller->controlService(SUBSCRIBE_TO_MOT6D, controller->names.at(index));
				}
			}
		}
	}
	return 0;
}



