
/*
*
*		Includes
*
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <winsvc.h>
#include <comdef.h>
#include <iostream>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <sstream>
#define _USE_MATH_DEFINES
#include <math.h>

#pragma comment(lib, "Urlmon.lib")
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#pragma comment (lib, "Shlwapi.lib")

/*
*
*				Enums and Structs for nControl and OpenSpatial Events
*
*/
enum ServiceControlCode {
	SUBSCRIBE_TO_POINTER = 128,
	SUBSCRIBE_TO_GESTURE = 129,
	SUBSCRIBE_TO_POSE6D = 130,
	SUBSCRIBE_TO_BUTTON = 131,
	SHUTDOWN_NOD = 132,
	GET_CONNECTED_NAMES = 133,
	UNSUBSCRIBE_TO_POINTER = 134,
	UNSUBSCRIBE_TO_GESTURE = 135,
	UNSUBSCRIBE_TO_POSE6D = 136,
	UNSUBSCRIBE_TO_BUTTON = 147,
	RECENTER_NOD = 137,
	RECALIBRATE_NOD = 138,
	FLIP_Y_NOD = 139,
	FLIP_ROT_NOD = 140,
	SET_SCREEN_RES = 141,
	SET_INPUT_QUEUE_DEPTH = 142,
	SET_TAP_TIME = 143,
	SET_DOUBLE_TAP_TIME = 144,
	REFRESH_SERVICE = 145,
	SUBSCRIBE_TO_MOT6D = 155,
	SUBSCRIBE_TO_GAMECONTROL = 156,
	UNSUBSCRIBE_TO_MOT6D = 157,
	UNSUBSCRIBE_TO_GAMECONTROL = 158,
	OTA = 146,
	GET_BATTERY = 148,
	GET_FIRMWARE = 149,
	SET_MODE_POINTER = 150,
	SET_MODE_GAMEPAD_L = 151,
	SET_MODE_GAMEPAD_R = 152,
	SET_MODE_FREEPOINTER = 153,
	GET_MODE = 154
};
enum GestureOperations {
	G_OP_SCROLL = 0x0001,
	G_OP_DIRECTION =  0x0002,
};
enum GestureActions {
	GRIGHT = 0x01,
	GLEFT = 0x02,
	GDOWN = 0x03,
	GUP = 0x04,
	GCW = 0x05,
	GCCW = 0x06,
};
enum SliderActions {
 SLIDE_LEFT = 0x01,
 SLIDE_RIGHT = 0x02,
};
enum ButtonEventType {
	TOUCH0_DOWN,
	TOUCH0_UP,
	TOUCH1_UP,
	TOUCH1_DOWN,
	TOUCH2_DOWN,
	TOUCH2_UP,
	TACTILE1_DOWN,
	TACTILE1_UP,
	TACTILE0_DOWN,
	TACTILE0_UP
};
enum GestureEventType {
	SWIPE_DOWN,
	SWIPE_LEFT,
	SWIPE_RIGHT,
	SWIPE_UP,
	CW,
	CCW,
	SLIDER_LEFT,
	SLIDER_RIGHT
};
struct PointerEvent
{
	int x;
	int y;
	int sender;
};
struct GestureEvent
{
	int gestureType;
	int sender;
};
struct ButtonEvent
{
	int buttonEventType;
	int sender;
};
struct Pose6DEvent
{
	int x;
	int y;
	int z;
	float yaw;
	float pitch;
	float roll;
	int sender;
};
struct GameControlEvent
{
	int x;
	int y;
	int trigger;
	int sender;
};
struct Motion6DEvent
{
	float accelX;
	float accelY;
	float accelZ;
	float gyroX;
	float gyroY;
	float gyroZ;
	int sender;
};
struct FirmwareVersion
{
	int major = -1;
	int minor = -1;
	int subminor = -1;
};
struct DeviceManager
{
	BOOL isSubscribedPointer = false;
	BOOL isSubscribedPose6D = false;
	BOOL isSubscribedGestures = false;
	BOOL isSubscribedButtons = false;
	BOOL isSubscribedMotion6D = false;
	BOOL isSubscribedGameControl = false;
	FirmwareVersion firmwareVersion;
	int batteryPercentage = -1;
	int mode = -1;
};
enum charType : BYTE {
	POINTER2D,
	GESTURE,
	POSE6D,
	MOT6D,
	BUTTON,
	GAMECONTROL
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

const unsigned int BUTTON_UP = 2;
const unsigned int BUTTON_DOWN = 1;
const unsigned int DEV = 0;
const unsigned int ALPHA = 1;

/*
*		
*			Methods which need their own thread for execution
*
*/
//Methods to open sockets on different threads for nonblocking
DWORD WINAPI openDataSocket(LPVOID lpParam);
DWORD WINAPI listenNameSocket(LPVOID lpParam);
int decodeAndSendEvents(unsigned char* bytes, int numBytes);
//start service on a different thread to not block anyone calling while looking for rings
DWORD WINAPI startServiceThread(LPVOID lpParam);
DWORD WINAPI controlServiceAsync(LPVOID lpParam);


/*
*
*			Open Spatial Service Controller Class
*
*/

class OpenSpatialDelegate
{
public:
	virtual void pointerEventFired(PointerEvent event) = 0;
	virtual void gestureEventFired(GestureEvent event) = 0;
	virtual void pose6DEventFired(Pose6DEvent event) = 0;
	virtual void buttonEventFired(ButtonEvent event) = 0;
	virtual void gameControlEventFired(GameControlEvent event) = 0;
	virtual void motion6DEventFired(Motion6DEvent event) = 0;
	virtual void setupComplete(){}
	std::vector<std::string> names;
};

class OpenSpatialServiceController
{
public:
	OpenSpatialServiceController();
	~OpenSpatialServiceController();
	int setupService();
	std::vector<std::string> names;
	std::map<std::string, DeviceManager> deviceMap;
	std::vector<std::string> getNames();
	std::string getNameAt(int i);
	BOOL setup = false;
	void controlService(int action, std::string name);
	void setDelegate(OpenSpatialDelegate* del);
	SC_HANDLE serviceManager;
	void setBufferPackets(BOOL set);

	//Service Control Methods
	void subscribeToPointer(std::string name);
	void subscribeToGesture(std::string name);
	void subscribeToButton(std::string name);
	void subscribeToPose6D(std::string name);
	void subscribeToMotion6D(std::string name);
	void subscribeToGameControl(std::string name);
	void unsubscribeToPose6D(std::string name);
	void unsubscribeToPointer(std::string name);
	void unsubscribeToGesture(std::string name);
	void unsubscribeToButton(std::string name);
	void unsubscribeToMotion6D(std::string name);
	void unsubscribeToGameControl(std::string name);
	void shutdown(std::string name);
	void recenter(std::string name);
	void recalibrate(std::string name);
	void flipY(std::string name);
	void flipRot(std::string name);
	void refreshService();

	int waitForServiceStatus(DWORD statusTo, SC_HANDLE service);
	SC_HANDLE OSService;
private:
	void ClearGlobalVariables();
	DWORD openNameSocket();
	void sendName(std::string name);
	HANDLE dataThreadHandle;
	HANDLE nameThreadHandle;
	DWORD dataThreadID;
	DWORD nameThreadID;
};
