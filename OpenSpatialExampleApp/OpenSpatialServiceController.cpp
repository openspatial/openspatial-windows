#include "stdafx.h"
#include "OpenSpatialServiceController.h"
/*
*
*			Global Variables for OpenSpatialServiceController
*
*/
SOCKET nameSocket;
SOCKET dataSocket;
DWORD portCount = 0;
std::vector<std::string> nameOutput;
OpenSpatialDelegate* delegate;
bool VERBOSE = false;
bool iterateBufferedPackets = false;
const int BUFFER_SIZE = 512;
const char* DEFAULT_PORT = "19999";
const char* DATA_PORT = "20001";
const char* INFO_PORT = "20000";
const char* OTA_PORT = "20002";
SOCKET infoSocket;
SOCKET otaSocket;
int currBatt = -1;
int currFirm = -1;
int currMode = -1;
int otaStatus = OTA_NOT_STARTED;
int otaPercentage = -1;
CRITICAL_SECTION versionSection;
CRITICAL_SECTION serviceSynchronizer;
BOOL downloadComplete = false;

/*
*
*				Constructors and Setup
*
*/
OpenSpatialServiceController::OpenSpatialServiceController() :
	dataThreadHandle(0),
	nameThreadHandle(0),
	dataThreadID(0),
	nameThreadID(0)
{
	InitializeCriticalSectionAndSpinCount(&serviceSynchronizer, 0x0000400);
	ClearGlobalVariables();
	CreateThread(
		NULL,
		0,
		startServiceThread,
		(LPVOID) this,
		0,
		NULL);

}
DWORD WINAPI startServiceThread(LPVOID lpParam)
{
	EnterCriticalSection(&serviceSynchronizer);
	OpenSpatialServiceController* cont = (OpenSpatialServiceController*)lpParam;
	cont->setupService();
	LeaveCriticalSection(&serviceSynchronizer);
	return 0;
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
			if (VERBOSE)
				std::cout << "COULD NOT OPEN SERVICE" << std::endl;
			return -1;
		}
		else
		{
			if (VERBOSE)
				std::cout << "Found Service" << std::endl;
		}
	}
	else
	{
		if (VERBOSE)
			std::cout << "COULD NOT OPEN MANAGER %d" << GetLastError() << std::endl;
		return -1;
	}

	int check = waitForServiceStatus(SERVICE_RUNNING, OSService);
	if(check < 0)
	{
		return -1;
	}
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
		for (size_t i = 0; i < nameOutput.size(); i++)
		{
			int in = nameOutput.at(i).find("END");
			if (in < 0)
			{
				names.push_back(nameOutput.at(i));
				std::cout << names.at(i) << " ";
				DeviceManager manager;
				deviceMap[names.at(i)] = manager;
			}
			else
			{
				nameOutput.clear();
				break;
			}
		}
	}
	else
	{
		if (VERBOSE)
			std::cout << "ERROR GET NAMES " << GetLastError() << std::endl;
		return -1;
	}
	setup = true;
	if(delegate)
	{
		delegate->setupComplete();
	}
	return 0;
}
std::vector<std::string> OpenSpatialServiceController::getNames()
{
	return names;
}
std::string OpenSpatialServiceController::getNameAt(int i)
{
	return names.at(i);
}
int OpenSpatialServiceController::waitForServiceStatus(DWORD statusTo, SC_HANDLE service)
{
	BOOL isInStatus = false;
	SERVICE_STATUS status;

	while (!isInStatus)
	{
		BOOL query = QueryServiceStatus(service, &status);
		if (!query)
		{
			if (VERBOSE)
				std::cout << "ERROR WAITING QUERY " << GetLastError() << std::endl;
			return -1;
		}
		else
		{
			if (status.dwCurrentState == statusTo)
			{
				isInStatus = true;
			}
		}
	}
	return 0;
}

/*
*
*				Destructors and destruction methods
*
*/
OpenSpatialServiceController::~OpenSpatialServiceController()
{
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
	DeleteCriticalSection(&serviceSynchronizer);
}
void OpenSpatialServiceController::ClearGlobalVariables()
{
	EnterCriticalSection(&serviceSynchronizer);
	nameOutput.clear();
	LeaveCriticalSection(&serviceSynchronizer);
	portCount = 0;
}

/*
*
*				Setters for Service Options
*
*/
void OpenSpatialServiceController::setBufferPackets(BOOL set)
{
	iterateBufferedPackets = set;
}
void OpenSpatialServiceController::setDelegate(OpenSpatialDelegate* del)
{
	delegate = del;
}

/*
*
*				Socket Opening and Listening Methods
*
*/
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
		if (VERBOSE)
			std::cout << "WSAStartup failed with error: " << iResult << std::endl;
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo("127.0.0.1", DATA_PORT, &hints, &result);
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
			if (VERBOSE)
				std::cout << "socket failed with error: " <<  WSAGetLastError() << std::endl;
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
		if (VERBOSE)
			std::cout << "Unable to connect to server!" << std::endl;
		WSACleanup();
		return 1;
	}
	if (VERBOSE)
		std::cout << "data connected" << std::endl;
	int already_read = 0;
	while (true)
	{
		if (VERBOSE)
		{
			if (already_read)
			{
				std::cout << "reading at most " << recvbuflen - already_read <<
					std::endl;
			}
		}
		iResult = recv(dataSocket, (char*)recvbuf + already_read, recvbuflen - already_read, 0);
		iResult += already_read;
		if (iResult > 0)
		{
			int undecoded = decodeAndSendEvents(recvbuf, iResult);
			if (undecoded)
			{
				if (VERBOSE)
				{
					std::cout << undecoded << " bytes left undecoded!" << std::endl;
					for (int i = iResult - undecoded; i < iResult; i++)
					{
						printf("%.2x", recvbuf[i]);
					}
					printf("\n");
				}
			}
			already_read = undecoded;
			// Put the unprocessed bytes at the front of recvbuf for the next call
			if ((undecoded > 0) && (iResult - undecoded) >= 0)
			{
				memmove(recvbuf, recvbuf + (iResult - undecoded), undecoded);
			}
		}
		else if (iResult == 0)
		{
			if (VERBOSE)
			{
				std::cout << "Connection closed" << std::endl;
			}
		}
	}
	return 0;
}
int decodeAndSendEvents(unsigned char* bytes, int numBytes)
{
	static bool wasPacketMaxSize = false;
	if (VERBOSE)
	{
		if (numBytes == BUFFER_SIZE) {
			std::cout << "decodeAndSendEvents entered with 512 bytes!" << std::endl;
			for (int i = 0; i < 40; i++) {
				printf("%.2x", bytes[i]);
			}
			printf("\n");
		}
	}
	if (!iterateBufferedPackets)
	{
		if (numBytes >= BUFFER_SIZE)
		{
			wasPacketMaxSize = true;
			return 0;
		}
		if (wasPacketMaxSize)
		{
			wasPacketMaxSize = false;
			return 0;
		}
	}
	int npose = 0;
	int nmotion = 0;
	int ngesture = 0;
	int nbutton = 0;
	int ngame = 0;
	int npointer = 0;
	static int lt0 = 0;
	static int lt1 = 0;
	static int lt2 = 0;
	static int lta0 = 0;
	static int lta1 = 0;
	if (numBytes <= 0) {
		return 0;
	}
	if (bytes == nullptr) {
		return 0;
	}

	if (NULL == delegate) {
		return 0;
	}
	int counter = 0;
	while (counter < numBytes) {

		charType type = (charType)bytes[0];
		int absorbed = 0;

		if (type == POINTER2D)
		{

			if ((numBytes - counter) < 4) {
				if (VERBOSE) {
					std::cout << "Insufficient byte size " << (numBytes - counter) << " for pointer packet" << std::endl;
				}
				return (numBytes - counter);
			}
			npointer++;
			// Pointer 2d
			short x = bytes[counter + 1] | bytes[counter + 2] << 8;
			short y = bytes[counter + 3] | bytes[counter + 4] << 8;
			BYTE id = bytes[counter + 5];

			PointerEvent pEvent;
			pEvent.x = x;
			pEvent.y = y;
			pEvent.sender = id;

			delegate->pointerEventFired(pEvent);
			counter += 6;
		}
		else if (type == POSE6D)
		{
			if ((numBytes - counter) < 14) {
				if (VERBOSE) {
					std::cout << "Insufficient byte size " << (numBytes - counter) << " for pose6d packet" << std::endl;
				}
				return  (numBytes - counter);
			}
			npose++;
			short int x = bytes[counter + 1] | (bytes[counter + 2] << 8);
			short int y = bytes[counter + 3] | (bytes[counter + 4] << 8);
			short int z = bytes[counter + 5] | (bytes[counter + 6] << 8);
			short int roll = ((int)bytes[counter + 7]) | (((int)bytes[counter + 8]) << 8);
			short int pitch = ((int)bytes[counter + 9]) | (((int)bytes[counter + 10]) << 8);
			short int yaw = ((int)bytes[counter + 11]) | (((int)bytes[counter + 12]) << 8);

			BYTE id = bytes[counter + 13];

			float rollf = (float)(roll << 16);
			float pitchf = (float)(pitch << 16);
			float yawf = (float)(yaw << 16);


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
			counter += 14;
		}
		else if (type == MOT6D)
		{

			if ((numBytes - counter) < 14) {
				if (VERBOSE) {
					std::cout << "Insufficient byte size " << (numBytes - counter) << " for pose6d packet" << std::endl;
				}
				return (numBytes - counter);
			}
			nmotion++;
			short int x = bytes[counter + 1] | (bytes[counter + 2] << 8);
			short int y = bytes[counter + 3] | (bytes[counter + 4] << 8);
			short int z = bytes[counter + 5] | (bytes[counter + 6] << 8);
			short int roll = ((int)bytes[counter + 7]) | (((int)bytes[counter + 8]) << 8);
			short int pitch = ((int)bytes[counter + 9]) | (((int)bytes[counter + 10]) << 8);
			short int yaw = ((int)bytes[counter + 11]) | (((int)bytes[counter + 12]) << 8);

			BYTE id = bytes[counter + 13];

			float accelX = ((float)x) / 8192;
			float accelY = ((float)y) / 8192;
			float accelZ = ((float)z) / 8192;

			float gyroX = (((float)roll) / 16.4) * (M_PI / 180);
			float gyroY = (((float)pitch) / 16.4) * (M_PI / 180);
			float gyroZ = (((float)yaw) / 16.4) * (M_PI / 180);

			Motion6DEvent mEvent;
			mEvent.accelX = accelX;
			mEvent.accelY = accelY;
			mEvent.accelZ = accelZ;
			mEvent.gyroX = gyroX;
			mEvent.gyroY = gyroY;
			mEvent.gyroZ = gyroZ;
			mEvent.sender = id;

			delegate->motion6DEventFired(mEvent);
			counter += 14;
		}
		else if (type == BUTTON)
		{

			if ((numBytes - counter) < 4) {
				if (VERBOSE) {
					std::cout << "Insufficient byte size " << (numBytes - counter) << " for button packet" << std::endl;
				}
				return (numBytes - counter);
			}
			nbutton++;
			short button = bytes[counter + 1] | (bytes[counter + 2] << 8);
			short touch0 = (button & 0x3);
			short touch1 = (button >> 2) & 0x3;
			short touch2 = (button >> 4) & 0x3;
			short tact0 = (button >> 6) & 0x3;
			short tact1 = (button >> 8) & 0x3;

			BYTE id = bytes[counter + 3];


			//PAW pulled all the else clauses because I found that the Down state was sticking frequently
			ButtonEvent bEvent;
			bEvent.sender = id;
			if (touch0 == BUTTON_DOWN && (lt0 != touch0))
			{
				bEvent.buttonEventType = TOUCH0_DOWN;
				delegate->buttonEventFired(bEvent);
			}
			if (touch0 == BUTTON_UP && (lt0 != touch0))
			{
				bEvent.buttonEventType = TOUCH0_UP;
				delegate->buttonEventFired(bEvent);
			}

			if (touch1 == BUTTON_DOWN && (lt1 != touch1))
			{
				bEvent.buttonEventType = TOUCH1_DOWN;
				delegate->buttonEventFired(bEvent);
			}
			if (touch1 == BUTTON_UP && (lt1 != touch1))
			{
				bEvent.buttonEventType = TOUCH1_UP;
				delegate->buttonEventFired(bEvent);
			}

			if (touch2 == BUTTON_DOWN && (lt2 != touch2))
			{
				bEvent.buttonEventType = TOUCH2_DOWN;
				delegate->buttonEventFired(bEvent);
			}
			if (touch2 == BUTTON_UP && (lt2 != touch2))
			{
				bEvent.buttonEventType = TOUCH2_UP;
				delegate->buttonEventFired(bEvent);
			}

			if (tact0 == BUTTON_DOWN && (lta0 != tact0))
			{
				bEvent.buttonEventType = TACTILE0_DOWN;
				delegate->buttonEventFired(bEvent);
			}
			if (tact0 == BUTTON_UP && (lta0 != tact0))
			{
				bEvent.buttonEventType = TACTILE0_UP;
				delegate->buttonEventFired(bEvent);
			}

			if (tact1 == BUTTON_DOWN && (lta1 != tact1))
			{
				bEvent.buttonEventType = TACTILE1_DOWN;
				delegate->buttonEventFired(bEvent);
			}
			if (tact1 == BUTTON_UP && (lta1 != tact1))
			{
				bEvent.buttonEventType = TACTILE1_UP;
				delegate->buttonEventFired(bEvent);
			}

			lt0 = touch0;
			lt1 = touch1;
			lt2 = touch2;
			lta0 = tact0;
			lta1 = tact1;

			counter += 4;
		}
		else if (type == GESTURE)
		{

			if ((numBytes - counter) < 5) {
				if (VERBOSE) {
					std::cout << "Insufficient byte size " << (numBytes - counter) << " for gesture packet" << std::endl;
				}
				return (numBytes - counter);
			}
			ngesture++;
			//gesture
			short opCode = bytes[counter + 1] | bytes[counter + 2] << 8;
			char data = bytes[counter + 3];

			BYTE id = bytes[counter + 4];

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
			counter += 5;
		}
		else if (type == GAMECONTROL)
		{

			if ((numBytes - counter) < 10) {
				if (VERBOSE) {
					std::cout << "Insufficient byte size " << (numBytes - counter) << " for game packet" << std::endl;
				}
				return (numBytes - counter);
			}
			ngame++;
			BYTE id = bytes[counter + 9];
			GameControlEvent gcEvent;
			gcEvent.x = bytes[counter + 1] | bytes[counter + 2] << 8;
			gcEvent.y = bytes[counter + 3] | bytes[counter + 4] << 8;
			gcEvent.trigger = bytes[counter + 5] | bytes[counter + 6] << 8;
			gcEvent.sender = id;
			delegate->gameControlEventFired(gcEvent);

			short button = bytes[counter + 0] | (bytes[counter  + 1] << 8);
			short touch0 = (button & 0x3);
			short touch1 = (button >> 2) & 0x3;
			short touch2 = (button >> 4) & 0x3;
			short tact0 = (button >> 6) & 0x3;
			short tact1 = (button >> 8) & 0x3;


			//PAW pulled all the else clauses because I found that the Down state was sticking frequently
			ButtonEvent bEvent;
			bEvent.sender = id;
			if (touch0 == BUTTON_DOWN && (lt0 != touch0))
			{
				bEvent.buttonEventType = TOUCH0_DOWN;
				delegate->buttonEventFired(bEvent);
			}
			if (touch0 == BUTTON_UP && (lt0 != touch0))
			{
				bEvent.buttonEventType = TOUCH0_UP;
				delegate->buttonEventFired(bEvent);
			}

			if (touch1 == BUTTON_DOWN && (lt1 != touch1))
			{
				bEvent.buttonEventType = TOUCH1_DOWN;
				delegate->buttonEventFired(bEvent);
			}
			if (touch1 == BUTTON_UP && (lt1 != touch1))
			{
				bEvent.buttonEventType = TOUCH1_UP;
				delegate->buttonEventFired(bEvent);
			}

			if (touch2 == BUTTON_DOWN && (lt2 != touch2))
			{
				bEvent.buttonEventType = TOUCH2_DOWN;
				delegate->buttonEventFired(bEvent);
			}
			if (touch2 == BUTTON_UP && (lt2 != touch2))
			{
				bEvent.buttonEventType = TOUCH2_UP;
				delegate->buttonEventFired(bEvent);
			}

			if (tact0 == BUTTON_DOWN && (lta0 != tact0))
			{
				bEvent.buttonEventType = TACTILE0_DOWN;
				delegate->buttonEventFired(bEvent);
			}
			if (tact0 == BUTTON_UP && (lta0 != tact0))
			{
				bEvent.buttonEventType = TACTILE0_UP;
				delegate->buttonEventFired(bEvent);
			}

			if (tact1 == BUTTON_DOWN && (lta1 != tact1))
			{
				bEvent.buttonEventType = TACTILE1_DOWN;
				delegate->buttonEventFired(bEvent);
			}
			if (tact1 == BUTTON_UP && (lta1 != tact1))
			{
				bEvent.buttonEventType = TACTILE1_UP;
				delegate->buttonEventFired(bEvent);
			}

			lt0 = touch0;
			lt1 = touch1;
			lt2 = touch2;
			lta0 = tact0;
			lta1 = tact1;
			counter += 10;
		}
		else {
			if (VERBOSE) {
				std::cout << "Unknown packet type " << (int)type << std::endl;
			}
			return 0;
		}
	}
	return 0;
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
		if (VERBOSE)
			std::cout << "WSAStartup failed with error: " << iResult << std::endl;
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		if (VERBOSE)
			std::cout << "getaddrinfo failed with error: " <<  iResult << std::endl;
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		nameSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (nameSocket == INVALID_SOCKET) {
			if (VERBOSE)
				std::cout << "socket failed with error: " <<  WSAGetLastError() << std::endl;
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
		if (VERBOSE)
			std::cout << "Unable to connect to server!" << std::endl;
		WSACleanup();
		return 1;
	}
	if (VERBOSE)
		std::cout << "name connected" << std::endl;

	return 0;
}
DWORD WINAPI listenNameSocket(LPVOID lpParam)
{
	char nameb[BUFFER_SIZE];
	while (true)
	{
		int recvbuflen = BUFFER_SIZE;
		int bytesRead = recv(nameSocket, nameb, recvbuflen, 0);
		std::string name = nameb;
		name = name.substr(0, bytesRead);
		nameOutput.push_back(name);
		ZeroMemory(nameb, BUFFER_SIZE);
	}
	return 0;
}

/*
*
*				Service Communication / Synchornization Methods
*
*/

struct serviceControllerStruct
{
	OpenSpatialServiceController* serv;
	int action;
	std::string name;
};

void OpenSpatialServiceController::controlService(int action, std::string name)
{
	serviceControllerStruct* controlStruct = new serviceControllerStruct;
	controlStruct->serv = this;
	controlStruct->action = action;
	controlStruct->name = name;
	if (setup)
	{
		CreateThread(
			NULL,
			0,
			controlServiceAsync,
			(LPVOID)controlStruct,
			0,
			NULL);
	}
}

DWORD WINAPI controlServiceAsync(LPVOID lpParam)
{
	serviceControllerStruct* controlStruct = (serviceControllerStruct*)lpParam;
	int action = controlStruct->action;
	std::string name = controlStruct->name;
	OpenSpatialServiceController* serv = controlStruct->serv;
	int serviceExists = serv->waitForServiceStatus(SERVICE_RUNNING, serv->OSService);
	if (serviceExists >= 0)
	{
		EnterCriticalSection(&serviceSynchronizer);
		switch (action)
		{
		case SUBSCRIBE_TO_BUTTON:
			serv->subscribeToButton(name);
			break;
		case SUBSCRIBE_TO_GESTURE:
			serv->subscribeToGesture(name);
			break;
		case SUBSCRIBE_TO_POINTER:
			serv->subscribeToPointer(name);
			break;
		case SUBSCRIBE_TO_POSE6D:
			serv->subscribeToPose6D(name);
			break;
		case SUBSCRIBE_TO_MOT6D:
			serv->subscribeToMotion6D(name);
			break;
		case SUBSCRIBE_TO_GAMECONTROL:
			serv->subscribeToGameControl(name);
			break;
		case UNSUBSCRIBE_TO_BUTTON:
			serv->unsubscribeToButton(name);
			break;
		case UNSUBSCRIBE_TO_GESTURE:
			serv->unsubscribeToGesture(name);
			break;
		case UNSUBSCRIBE_TO_POINTER:
			serv->unsubscribeToPointer(name);
			break;
		case UNSUBSCRIBE_TO_POSE6D:
			serv->unsubscribeToPose6D(name);
			break;
		case UNSUBSCRIBE_TO_MOT6D:
			serv->unsubscribeToMotion6D(name);
			break;
		case UNSUBSCRIBE_TO_GAMECONTROL:
			serv->unsubscribeToGameControl(name);
			break;
		case SHUTDOWN_NOD:
			serv->shutdown(name);
			break;
		case RECENTER_NOD:
			serv->recenter(name);
			break;
		case RECALIBRATE_NOD:
			serv->recalibrate(name);
			break;
		case FLIP_Y_NOD:
			serv->flipY(name);
			break;
		case FLIP_ROT_NOD:
			serv->flipRot(name);
			break;
		case REFRESH_SERVICE:
			serv->refreshService();
			break;
		default: break;
		}
		LeaveCriticalSection(&serviceSynchronizer);
	}
	delete controlStruct;
	return 0;
}

void OpenSpatialServiceController::sendName(std::string name)
{
	nameOutput.clear();
	send(nameSocket, name.c_str(), strlen(name.c_str()), 0);
}
void OpenSpatialServiceController::refreshService()
{
	LPSERVICE_STATUS status = static_cast<LPSERVICE_STATUS>(malloc(sizeof(SERVICE_STATUS)));
	BOOL bResult = ControlService(OSService, REFRESH_SERVICE, status);
	if (!bResult)
	{
		std::cout << "ERROR REFRESH " << GetLastError() << std::endl;;
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
		for (size_t i = 0; i < nameOutput.size(); i++)
		{
			int in = nameOutput.at(i).find("END");
			if (in < 0)
			{
				names.push_back(nameOutput.at(i));
				std::cout << names.at(i) << " ";
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
			for (size_t i = 0; i < names.size(); i++)
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
	if (delegate)
	{
		delegate->names.clear();
		for (size_t i = 0; i < names.size(); i++)
		{
			delegate->names.push_back(names.at(i));
		}
	}
}

/*
*
*				Subscription and Unsubscription
*
*/
void OpenSpatialServiceController::subscribeToPointer(std::string name)
{
	if (!deviceMap[name].isSubscribedPointer)
	{
		sendName(name);
		deviceMap[name].isSubscribedPointer = true;
		SERVICE_STATUS status;
		BOOL bResult = ControlService(OSService, SUBSCRIBE_TO_POINTER, &status);
		if (!bResult)
		{
			if (VERBOSE)
				std::cout << "ERROR SUBSCRIBE 2D " << GetLastError() << std::endl;
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
			if (VERBOSE)
				std::cout << "ERROR SUBSCRIBE BUTTON " <<  GetLastError() << std::endl;
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
			if (VERBOSE)
				std::cout << "ERROR SUBSCRIBE GESTURE " << GetLastError() << std::endl;;
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
			if (VERBOSE)
				std::cout << "ERROR SUBSCRIBE POSE 6D " << GetLastError() << std::endl;;
			//Handle errors
		}
	}
}
void OpenSpatialServiceController::subscribeToMotion6D(std::string name)
{
	if (!deviceMap[name].isSubscribedMotion6D)
	{
		sendName(name);
		deviceMap[name].isSubscribedMotion6D = true;
		SERVICE_STATUS status;
		BOOL bResult = ControlService(OSService, SUBSCRIBE_TO_MOT6D, &status);
		if (!bResult)
		{
			if (VERBOSE)
				std::cout << "ERROR SUBSCRIBE MOTION6D " << GetLastError() << std::endl;;
			//Handle errors
		}
	}
}
void OpenSpatialServiceController::subscribeToGameControl(std::string name)
{
	if (!deviceMap[name].isSubscribedGameControl)
	{
		sendName(name);
		deviceMap[name].isSubscribedGameControl = true;
		SERVICE_STATUS status;
		BOOL bResult = ControlService(OSService, SUBSCRIBE_TO_GAMECONTROL, &status);
		if (!bResult)
		{
			if (VERBOSE)
				std::cout << "ERROR SUBSCRIBE GAME " << GetLastError() << std::endl;;
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
			if (VERBOSE)
				std::cout << "ERROR UNSUBSCRIBE 2D " << GetLastError() << std::endl;;
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
			if (VERBOSE)
				std::cout << "ERROR UNSUBSCRIBE BUTTON " << GetLastError() << std::endl;;
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
			if (VERBOSE)
				std::cout << "ERROR UNSUBSCRIBE GESTURE " << GetLastError() << std::endl;;
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
			if (VERBOSE)
				std::cout << "ERROR UNSUBSCRIBE POSE 6D " << GetLastError() << std::endl;;
			//Handle errors
		}
	}
}
void OpenSpatialServiceController::unsubscribeToMotion6D(std::string name)
{
	if (deviceMap[name].isSubscribedMotion6D)
	{
		sendName(name);
		deviceMap[name].isSubscribedMotion6D = false;
		SERVICE_STATUS status;
		BOOL bResult = ControlService(OSService, UNSUBSCRIBE_TO_MOT6D, &status);
		if (!bResult)
		{
			if (VERBOSE)
				std::cout << "ERROR UNSUBSCRIBE MOTION6D " << GetLastError() << std::endl;
			//Handle errors
		}
	}
}
void OpenSpatialServiceController::unsubscribeToGameControl(std::string name)
{
	if (deviceMap[name].isSubscribedGameControl)
	{
		sendName(name);
		deviceMap[name].isSubscribedGameControl = false;
		SERVICE_STATUS status;
		BOOL bResult = ControlService(OSService, UNSUBSCRIBE_TO_GAMECONTROL, &status);
		if (!bResult)
		{
			if (VERBOSE)
				std::cout << "ERROR SUBSCRIBE GAME " << GetLastError() << std::endl;
			//Handle errors
		}
	}
}

/*
*
*				nControl Methods
*
*/
void OpenSpatialServiceController::shutdown(std::string name)
{
	sendName(name);
	SERVICE_STATUS status;
	BOOL bResult = ControlService(OSService, SHUTDOWN_NOD, &status);
	if (!bResult)
	{
		if (VERBOSE)
			std::cout << "ERROR SHUTDOWN " << GetLastError() << std::endl;;
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
		if (VERBOSE)
			std::cout << "ERROR RECENTER " << GetLastError() << std::endl;;
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
		if (VERBOSE)
			std::cout << "ERROR RECALIBRATE " << GetLastError() << std::endl;;
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
		if (VERBOSE)
			std::cout << "ERROR flipY " << GetLastError() << std::endl;;
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
		if (VERBOSE)
			std::cout << "ERROR flipRot " << GetLastError() << std::endl;;
		//Handle errors
	}
}
