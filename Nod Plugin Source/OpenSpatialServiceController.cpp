#include "stdafx.h"
#include "OpenSpatialServiceController.h"
#include <sstream>

SOCKET nameSocket;
SOCKET dataSocket;

DWORD portCount = 0;

std::vector<std::string> nameOutput;


OpenSpatialDelegate* delegate;

//PAW grasping at straws here, figured it couldn't hurt to make not print
const bool buildingForUnity = false;

OpenSpatialServiceController::OpenSpatialServiceController() :
	dataThreadHandle(0),
	nameThreadHandle(0),
	dataThreadID(0),
	nameThreadID(0)
{
	ClearGlobalVariables();

	if (setupService() < 0)
	{
		if (!buildingForUnity)
			printf("ERROR SETUP PLEASE TRY AGAIN");
		return;
	}
}

OpenSpatialServiceController::~OpenSpatialServiceController()
{
	for (int i = 0; i < names.size(); i++)
	{
		unsubscribeToButton(names.at(i));
		Sleep(500);
		unsubscribeToGesture(names.at(i));
		Sleep(500);
		unsubscribeToPointer(names.at(i));
		Sleep(500);
		unsubscribeToPose6D(names.at(i));
	}
	int result = CloseServiceHandle(OSService);
	if (result == 0) {
		//PAW something went wrong need to deal with in.
		// GetLastError()
	}
	DWORD exitCode;

	char* end = "closeport";

	send(dataSocket, end, strlen(end), 0);

	GetExitCodeThread(dataThreadHandle, &exitCode);
	TerminateThread(dataThreadHandle, exitCode);

	GetExitCodeThread(nameThreadHandle, &exitCode);
	TerminateThread(nameThreadHandle, exitCode);


	ClearGlobalVariables();
}

void OpenSpatialServiceController::ClearGlobalVariables()
{
	nameOutput.clear();

	portCount = 0;
}

void OpenSpatialServiceController::setDelegate(OpenSpatialDelegate* del)
{
	delegate = del;
}

int OpenSpatialServiceController::setupService()
{

	nameSocket = INVALID_SOCKET;
	dataSocket = INVALID_SOCKET;

	//get service manager and open service
	serviceManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | GENERIC_READ);
	if (serviceManager != NULL)
	{
		OSService = OpenService(serviceManager, L"OpenSpatialService", GENERIC_READ | READ_CONTROL | SERVICE_USER_DEFINED_CONTROL);
		if (OSService == NULL)
		{
			if (!buildingForUnity)
				printf("COULD NOT OPEN SERVICE");
			return -1;
		}
		else
		{
			if (!buildingForUnity)
				printf("Found Service\n");
		}
	}
	else
	{
		if (!buildingForUnity)
			printf("COULD NOT OPEN MANAGER %d", GetLastError());
		return -1;
	}

	waitForServiceStatus(SERVICE_RUNNING, OSService);

	//Connect To Socket
	dataThreadHandle = CreateThread(
		NULL,
		0,
		openDataSocket,
		NULL,
		0,
		&dataThreadID);
	openNameSocket();
	nameThreadHandle = CreateThread(
		NULL,
		0,
		listenNameSocket,
		NULL,
		0,
		&nameThreadID);

	SERVICE_STATUS status;
	//Refresh Service
	BOOL bResult = ControlService(OSService, REFRESH_SERVICE, &status);
	if (bResult)
	{
		Sleep(1000);
	}
	else
	{
		OutputDebugString(L"Error Refreshing");
		return -1;
	}
	//Get Names of connected devices
	bResult = ControlService(OSService, GET_CONNECTED_NAMES, &status);
	BOOL waitForNames = true;

	
	if (bResult)
	{
		while (waitForNames)
		{
			if (nameOutput.size() > 0)
			{
				if (nameOutput.at(nameOutput.size() - 1).find("END") >= 0)
				{
					waitForNames = false;
				}
			}
		}
		for (int i = 0; i < (int)nameOutput.size(); i++)
		{
			std::wostringstream os;
			os << nameOutput.at(i).c_str();
			int in = nameOutput.at(i).find("END");
			if (in < 0)
			{
				names.push_back(nameOutput.at(i));
				printf("%s, ", names.at(i).c_str());
			}
			else
			{
				nameOutput.clear();
				break;
			}
		}
		for (int i = 0; i < names.size(); i++)
		{
			DeviceManager manager;
			deviceMap[names.at(i)] = manager;
		}
	}
	else
	{
		if (!buildingForUnity)
			printf("ERROR GET NAMES %d", GetLastError());
		return -1;
	}
	return 0;
}

std::vector<std::string> OpenSpatialServiceController::getNames()
{
	return names;
}

void OpenSpatialServiceController::waitForServiceStatus(DWORD statusTo, SC_HANDLE service)
{
	BOOL isInStatus = false;
	SERVICE_STATUS status;

	while (!isInStatus)
	{
		//PAW documentation for QueryServiceStatus says "superseded by the QueryServiceStatusEx function." should we switch?
		BOOL query = QueryServiceStatus(service, &status);
		if (!query)
		{
			if (!buildingForUnity)
				printf("ERROR WAITING QUERY %d", GetLastError());
		}
		else
		{
			if (status.dwCurrentState == statusTo)
			{
				isInStatus = true;
			}
		}
	}
}

DWORD WINAPI openDataSocket(LPVOID lpParam)
{
	WSADATA wsaData;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	unsigned char recvbuf[BUFFER_SIZE];
	int iResult;
	int recvbuflen = BUFFER_SIZE;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		if (!buildingForUnity)
			printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	int port = DATA_PORT;
	std::ostringstream os;
	os << port;
	// Resolve the server address and port
	iResult = getaddrinfo("127.0.0.1", (PCSTR)os.str().c_str(), &hints, &result);
	if (iResult != 0) {
		OutputDebugString(TEXT("getaddrinfo failed with error: %d\n"));
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		dataSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (dataSocket == INVALID_SOCKET) {
			if (!buildingForUnity)
				printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(dataSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(dataSocket);
			dataSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (dataSocket == INVALID_SOCKET) {
		if (!buildingForUnity)
			printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}
	if (!buildingForUnity)
		printf("data connected\n");

	while (true)
	{
			iResult = recv(dataSocket, (char*)recvbuf, recvbuflen, 0);
			if (iResult > 0)
			{
				decodeAndSendEvents(recvbuf, iResult);
			}
			else if (iResult == 0) {
				if (!buildingForUnity)
					printf("Connection closed\n");
			}
	}
	return 0;
}

int lt0 = 0;
int lt1 = 0;
int lt2 = 0;
int lta0 = 0;
int lta1 = 0;

void decodeAndSendEvents(unsigned char* bytes, int numBytes)
{
	if (NULL == delegate)
		return;

	if (numBytes == 6)
	{
		// Pointer 2d
		short x = bytes[0] | bytes[1] << 8;
		short y = bytes[2] | bytes[3] << 8;
		short id = bytes[4] | bytes[5] << 8;

		PointerEvent pEvent;
		pEvent.x = x;
		pEvent.y = y;
		pEvent.sender = id;
		
		delegate->pointerEventFired(pEvent);
	}
	else if (numBytes == 14)
	{
		//Pose 6d
		short int x = bytes[0] | (bytes[1] << 8);
		short int y = bytes[2] | (bytes[3] << 8);
		short int z = bytes[4] | (bytes[5] << 8);
		
		int roll = ((int)bytes[6]) | (((int)bytes[7]) << 8);
		float rollf = (float)(roll << 16);
		int pitch = ((int)bytes[8]) | (((int)bytes[9]) << 8);
		float pitchf = (float)(pitch << 16);
		int yaw = ((int)bytes[10]) | (((int)bytes[11]) << 8);
		float yawf = (float)(yaw << 16);		
		
		short id = bytes[12] | bytes[13] << 8;

		rollf = rollf / (1 << 29);
		pitchf = pitchf / (1 << 29);
		yawf = yawf / (1 << 29);

		Pose6DEvent pEvent;
		pEvent.x = x;
		pEvent.y = y;
		pEvent.z = z;
		pEvent.roll = rollf;
		pEvent.pitch = pitchf;
		pEvent.yaw = yawf;
		pEvent.sender = id;

		delegate->pose6DEventFired(pEvent);		
	}
	else if (numBytes == 4)
	{
		short button = bytes[0] | (bytes[1] << 8);
		short touch0 = (button & 0x3);
		short touch1 = (button >> 2) & 0x3;
		short touch2 = (button >> 4) & 0x3;
		short tact0 = (button >> 6) & 0x3;
		short tact1 = (button >> 8) & 0x3;

		short id = bytes[2] | bytes[3] << 8;		

		
		//PAW pulled all the else clauses because I found that the Down state was sticking frequently

		if (touch0 == BUTTON_DOWN && (lt0 != touch0))
		{
			ButtonEvent bEvent;
			bEvent.buttonEventType = TOUCH0_DOWN;
			bEvent.sender = id;
			delegate->buttonEventFired(bEvent);
		}
		if (touch0 == BUTTON_UP && (lt0 != touch0))
		{
			ButtonEvent bEvent;
			bEvent.buttonEventType = TOUCH0_UP;
			bEvent.sender = id;			
			delegate->buttonEventFired(bEvent);
		}

		if (touch1 == BUTTON_DOWN && (lt1 != touch1))
		{
			ButtonEvent bEvent;
			bEvent.buttonEventType = TOUCH1_DOWN;
			bEvent.sender = id;
			delegate->buttonEventFired(bEvent);
		}
		if (touch1 == BUTTON_UP && (lt1 != touch1))
		{
			ButtonEvent bEvent;
			bEvent.buttonEventType = TOUCH1_UP;
			bEvent.sender = id;
			delegate->buttonEventFired(bEvent);
		}

		if (touch2 == BUTTON_DOWN && (lt2 != touch2))
		{
			ButtonEvent bEvent;
			bEvent.buttonEventType = TOUCH2_DOWN;
			bEvent.sender = id;
			delegate->buttonEventFired(bEvent);
		}
		if (touch2 == BUTTON_UP && (lt2 != touch2))
		{
			ButtonEvent bEvent;
			bEvent.buttonEventType = TOUCH2_UP;
			bEvent.sender = id;
			delegate->buttonEventFired(bEvent);
		}

		if (tact0 == BUTTON_DOWN && (lta0 != tact0))
		{
			ButtonEvent bEvent;
			bEvent.buttonEventType = TACTILE0_DOWN;
			bEvent.sender = id;
			delegate->buttonEventFired(bEvent);
		}
		if (tact0 == BUTTON_UP && (lta0 != tact0))
		{
			ButtonEvent bEvent;
			bEvent.buttonEventType = TACTILE0_UP;
			bEvent.sender = id;
			delegate->buttonEventFired(bEvent);
		}

		if (tact1 == BUTTON_DOWN && (lta1 != tact1))
		{
			ButtonEvent bEvent;
			bEvent.buttonEventType = TACTILE1_DOWN;
			bEvent.sender = id;
			delegate->buttonEventFired(bEvent);
		}
		if (tact1 == BUTTON_UP && (lta1 != tact1))
		{
			ButtonEvent bEvent;
			bEvent.buttonEventType = TACTILE1_UP;
			bEvent.sender = id;
			delegate->buttonEventFired(bEvent);
		}

		lt0 = touch0;
		lt1 = touch1;
		lt2 = touch2;
		lta0 = tact0;
		lta1 = tact1;

		if (!buildingForUnity)
			printf("\n");
	}
	else if (numBytes == 5)
	{
		//gesture
		short opCode = bytes[0] | bytes[1] << 8;
		char data = bytes[2];

		short id = bytes[3] | bytes[4];

		GestureEvent gEvent;
		gEvent.gestureType = -1;
		gEvent.sender = -1;

		if (opCode == G_OP_DIRECTION)
		{
			if (data == GUP)
			{
				gEvent.gestureType = SWIPE_UP;
				gEvent.sender = id;
			}
			else if (data == GDOWN)
			{
				gEvent.gestureType = SWIPE_DOWN;
				gEvent.sender = id;
			}
			else if (data == GLEFT)
			{
				gEvent.gestureType = SWIPE_LEFT;
				gEvent.sender = id;
			}
			else if (data == GRIGHT)
			{
				gEvent.gestureType = SWIPE_RIGHT;
				gEvent.sender = id;
			}
			else if (data == GCW)
			{
				gEvent.gestureType = CW;
				gEvent.sender = id;
			}
			else if (data == GCCW)
			{
				gEvent.gestureType = CCW;
				gEvent.sender = id;
			}
		}
		else if (opCode == G_OP_SCROLL)
		{
			if (data == SLIDE_LEFT)
			{
				gEvent.gestureType = SLIDER_LEFT;
				gEvent.sender = id;
			}
			else if (data == SLIDE_RIGHT)
			{
				gEvent.gestureType = SLIDER_RIGHT;
				gEvent.sender = id;
			}
		}

		delegate->gestureEventFired(gEvent);
	}
}

DWORD OpenSpatialServiceController::openNameSocket()
{
	WSADATA wsaData;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	//PAW not using this?
	//char recvbuf[BUFFER_SIZE];
	int iResult;
	int recvbuflen = BUFFER_SIZE;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		if (!buildingForUnity)
			printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	int port = DEFAULT_PORT - portCount;
	std::ostringstream os;
	os << port;
	// Resolve the server address and port
	iResult = getaddrinfo("127.0.0.1", os.str().c_str(), &hints, &result);
	if (iResult != 0) {
		if (!buildingForUnity)
			printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		nameSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (nameSocket == INVALID_SOCKET) {
			if (!buildingForUnity)
				printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(nameSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(nameSocket);
			nameSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (nameSocket == INVALID_SOCKET) {
		if (!buildingForUnity)
			printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}
	if (!buildingForUnity)
		printf("name connected\n");

	return 0;
}

DWORD WINAPI listenNameSocket(LPVOID lpParam)
{
	while (true)
	{
		char nameb[BUFFER_SIZE];
		int recvbuflen = BUFFER_SIZE;
		recv(nameSocket, nameb, recvbuflen, 0);
		std::string name = nameb;
		int index = name.find("Ì");
		name = name.substr(0, index);
		nameOutput.push_back(name);
		ZeroMemory(nameb, BUFFER_SIZE);
	}
	return 0;
}

void OpenSpatialServiceController::sendName(std::string name)
{
	nameOutput.clear();
	send(nameSocket, name.c_str(), strlen(name.c_str()), 0);
	BOOL accepted = false;
	/*while (!accepted)
	{
		if (nameOutput.size() > 0)
		{
			if (nameOutput.at(nameOutput.size() - 1).find("acce") >= 0)
			{
				accepted = true;
				nameOutput.clear();
			}
		}
	}*/
}

void OpenSpatialServiceController::subscribeToPointer(std::string name)
{
	if (! deviceMap[name].isSubscribedPointer)
	{
		sendName(name);
		deviceMap[name].isSubscribedPointer = true;
		SERVICE_STATUS status;
		BOOL bResult = ControlService(OSService, SUBSCRIBE_TO_POINTER, &status);
		if (!bResult)
		{
			if (!buildingForUnity)
				printf("ERROR SUBSCRIBE 2D %d", GetLastError());
			//Handle errors
		}
	}
}

void OpenSpatialServiceController::subscribeToButton(std::string name)
{
	if (!deviceMap[name].isSubscribedButtons)
	{
		sendName(name);
		deviceMap[name].isSubscribedButtons = true;
		SERVICE_STATUS status;
		BOOL bResult = ControlService(OSService, SUBSCRIBE_TO_BUTTON, &status);
		if (!bResult)
		{
			if (!buildingForUnity)
				printf("ERROR SUBSCRIBE BUTTON %d", GetLastError());
			//Handle errors
		}
	}
}

void OpenSpatialServiceController::subscribeToGesture(std::string name)
{
	if (!deviceMap[name].isSubscribedGestures)
	{
		sendName(name);
		deviceMap[name].isSubscribedGestures = true;
		SERVICE_STATUS status;
		BOOL bResult = ControlService(OSService, SUBSCRIBE_TO_GESTURE, &status);
		if (!bResult)
		{
			if (!buildingForUnity)
				printf("ERROR SUBSCRIBE GESTURE %d", GetLastError());
			//Handle errors
		}
	}
}

void OpenSpatialServiceController::subscribeToPose6D(std::string name)
{
	if (!deviceMap[name].isSubscribedPose6D)
	{
		sendName(name);
		deviceMap[name].isSubscribedPose6D = true;
		SERVICE_STATUS status;
		BOOL bResult = ControlService(OSService, SUBSCRIBE_TO_POSE6D, &status);
		if (!bResult)
		{
			if (!buildingForUnity)
				printf("ERROR SUBSCRIBE POSE 6D %d", GetLastError());
			//Handle errors
		}
	}
}

void OpenSpatialServiceController::unsubscribeToPointer(std::string name)
{
	if (deviceMap[name].isSubscribedPointer)
	{
		sendName(name);
		deviceMap[name].isSubscribedPointer = false;
		SERVICE_STATUS status;
		BOOL bResult = ControlService(OSService, UNSUBSCRIBE_TO_POINTER, &status);
		if (!bResult)
		{
			if (!buildingForUnity)
				printf("ERROR SUBSCRIBE 2D %d", GetLastError());
			//Handle errors
		}
	}
}

void OpenSpatialServiceController::unsubscribeToButton(std::string name)
{
	if (deviceMap[name].isSubscribedButtons)
	{
		sendName(name);
		deviceMap[name].isSubscribedButtons = false;
		SERVICE_STATUS status;
		BOOL bResult = ControlService(OSService, UNSUBSCRIBE_TO_BUTTON, &status);
		if (!bResult)
		{
			if (!buildingForUnity)
				printf("ERROR SUBSCRIBE BUTTON %d", GetLastError());
			//Handle errors
		}
	}
}

void OpenSpatialServiceController::unsubscribeToGesture(std::string name)
{
	if (deviceMap[name].isSubscribedGestures)
	{
		sendName(name);
		deviceMap[name].isSubscribedGestures = false;
		SERVICE_STATUS status;
		BOOL bResult = ControlService(OSService, UNSUBSCRIBE_TO_GESTURE, &status);
		if (!bResult)
		{
			if (!buildingForUnity)
				printf("ERROR SUBSCRIBE GESTURE %d", GetLastError());
			//Handle errors
		}
	}
}

void OpenSpatialServiceController::unsubscribeToPose6D(std::string name)
{
	if (deviceMap[name].isSubscribedPose6D)
	{
		sendName(name);
		deviceMap[name].isSubscribedPose6D = false;
		SERVICE_STATUS status;
		BOOL bResult = ControlService(OSService, UNSUBSCRIBE_TO_POSE6D, &status);
		if (!bResult)
		{
			if (!buildingForUnity)
				printf("ERROR SUBSCRIBE POSE 6D %d", GetLastError());
			//Handle errors
		}
	}
}

void OpenSpatialServiceController::shutdown(std::string name)
{
	sendName(name);
	SERVICE_STATUS status;
	BOOL bResult = ControlService(OSService, SHUTDOWN_NOD, &status);
	if (!bResult)
	{
		if (!buildingForUnity)
			printf("ERROR SHUTDOWN %d", GetLastError());
		//Handle errors
	}
}

void OpenSpatialServiceController::recenter(std::string name)
{
	sendName(name);
	SERVICE_STATUS status;
	BOOL bResult = ControlService(OSService, RECENTER_NOD, &status);
	if (!bResult)
	{
		if (!buildingForUnity)
			printf("ERROR SHUTDOWN %d", GetLastError());
		//Handle errors
	}
}
void OpenSpatialServiceController::recalibrate(std::string name)
{
	sendName(name);
	SERVICE_STATUS status;
	BOOL bResult = ControlService(OSService, RECALIBRATE_NOD, &status);
	if (!bResult)
	{
		if (!buildingForUnity)
			printf("ERROR SHUTDOWN %d", GetLastError());
		//Handle errors
	}
}
void OpenSpatialServiceController::flipY(std::string name)
{
	sendName(name);
	SERVICE_STATUS status;
	BOOL bResult = ControlService(OSService, FLIP_Y_NOD, &status);
	if (!bResult)
	{
		if (!buildingForUnity)
			printf("ERROR SHUTDOWN %d", GetLastError());
		//Handle errors
	}
}
void OpenSpatialServiceController::flipRot(std::string name)
{
	sendName(name);
	SERVICE_STATUS status;
	BOOL bResult = ControlService(OSService, FLIP_ROT_NOD, &status);
	if (!bResult)
	{
		if (!buildingForUnity)
			printf("ERROR SHUTDOWN %d", GetLastError());
		//Handle errors
	}
}

void OpenSpatialServiceController::refreshService()
{
	LPSERVICE_STATUS status = static_cast<LPSERVICE_STATUS>(malloc(sizeof(SERVICE_STATUS)));
	BOOL bResult = ControlService(OSService, REFRESH_SERVICE, status);
	if (!bResult)
	{
		printf("ERROR SHUTDOWN %d", GetLastError());
		//Handle errors
	}
	nameOutput.clear();
	names.clear();
	BOOL bResult2 = ControlService(OSService, GET_CONNECTED_NAMES, status);
	if (bResult2)
	{
		BOOL waitForNames = true;
		while (waitForNames)
		{
			if (nameOutput.size() > 0)
			{
				if (nameOutput.at(nameOutput.size() - 1).find("END") >= 0)
				{
					waitForNames = false;
				}
			}
		}
		for (int i = 0; i < nameOutput.size(); i++)
		{
			std::wostringstream os;
			os << nameOutput.at(i).c_str();
			OutputDebugString(os.str().c_str());
			int in = nameOutput.at(i).find("END");
			if (in < 0)
			{
				names.push_back(nameOutput.at(i));
				printf("%s, ", names.at(i).c_str());
			}
			else
			{
				nameOutput.clear();
				break;
			}
		}
		if (names.size() != deviceMap.size())
		{
			std::map<std::string, DeviceManager> newMap;
			for (int i = 0; i < names.size(); i++)
			{
				if (deviceMap.count(names.at(i)) > 0)
				{
					newMap[names.at(i)] = deviceMap[names.at(i)];
				}
				else
				{
					DeviceManager manager;
					newMap[names.at(i)] = manager;
				}
			}
			deviceMap = newMap;
		}
	}
	for (int i = 0; i < names.size(); i++)
	{
		delegate->names.push_back(names.at(i));
	}
}