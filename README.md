openspatial-windows
===================

Open Spatial Windows SDK

Open Spatial Service Instructions (v0.0.3)

Welcome to the Open Spatial Windows Service. Here are some basic instructions to get you started.

As of 9/21/2015 the Windows SDK and service has been majorly modified. We have rewritten the entire SDK and service to improve robustness. Please refer to this guide to learn how to use the new SDK

------- Part 1: The package -----------

First download or clone this repository. The repository has two folders. The Nod Installer and the Example Application. 

Double click the setup.exe from the NodInstaller and follow the dialogs to install the Service. The Service will start on system boot up. To make sure the service is running open a task manager, click the services tab and look for OpenSpatialService.

The example application is a VS2015 project which is already configured to use the Dynamic version of the SDK, The example application shows a simple way to connect to a device and retrieve data from it.

<<<<<<< HEAD
=======
Once installed, there should be a folder called "Nod Labs" in your C:\Program Files (x86)\ folder. Here is the relevant directory structure.
=======
Once installed, there should be a folder called "Nod Labs" in your C:\Program Files (x86)\ folder. There are 3 folders that
will contain the SDK dependencies:
>>>>>>> 8c23d43257c0edd6c68fdcf35a4a93972353fab4

 - Dynamic SDK
 - SDK
 - Unity
 
 The Dynamic SDK folder contains the ready to go DLL version of the SDK. This requires minimal linkage and is easy to get started with right away. It will also be what is covered in this guide.
 
 The SDK folder contains a static version of the SDK. This is much more difficult to link against and requires some 3rd party libraries. Only use this if your application cannot use dynamic libraries (DLL) and you absolutely need a static library. For more information about this please go to developer.nod.com or visit/post on https://forums.nod.com. This SDK will not be covered in this guide.
 
 The Unity folder contains the unity plugins for the service. These are just the bare unity plugins and do not include our Unity SDK. To get the full Unity SDK please visit our openspatial-unity repository. These have only been provided for versioning purposes. They should not need to be used and can be ignored.
 
------- Part 2: Using the Nod Configuration Application -----------

After installation a Nod Logo should appear in your system tray. Double Click this to open the Nod Configuration Application. This will show a list of connected Nod devices. This application is in early alpha so many of the features are incomplete. Currently it is only used to list the availible devices.

To use a Nod device with Windows your Nod device must be paired. For more instructions on how to pair a device to Windows visit developer.nod.com.

Any devices that are paired will appear in the list of devices.

------- Part 3: Development with the Dynamic SDK -----------

There are 4 parts to the dynamic SDK:
 - OpenSpatialDll.dll
 - OpenSpatialDll.lib
 - NodPlugin.h
 - Settings.h
 
The DLL and lib have been provided in a 32 bit and 64 bit version both under the Dynamic SDK directory in corresponding x86 and x86_64 directories. Use the appropriate one to build for the architecture you desire.

To link in the SDK add NodPlugin.h and Settings.h into your include path. In visual studio this can be as simple as placing the files in your project directory. Or, more optimally, going to your project settings and adding the directory in which you want to store these files to your "additional include directories" path. 

Next do the same with OpenSpatialDll.lib except place it in your library search path. 

Next place the DLL in the same directory as the executable your project will generate. For Visual Studio projects you may just place it within your project directory.

Now to begin coding:

There are 9 functions you can call with this DLL:

	NODPLUGIN_API bool NodInitialize(void(*evFiredFn)(NodEvent));
    NODPLUGIN_API bool NodShutdown(void);
    NODPLUGIN_API bool NodRefresh(void);

    NODPLUGIN_API int  NodNumRings(void);
    const NODPLUGIN_API char* NodGetRingName(int ringID);

    NODPLUGIN_API bool NodSubscribe(Modality mode, const char* deviceName);
    NODPLUGIN_API bool NodUnsubscribe(Modality mode, const char* deviceName);

    NODPLUGIN_API bool NodRequestDeviceInfo(const char* deviceName);
    NODPLUGIN_API bool NodChangeSetting(const char* deviceName, Settings setting, int args[], int numArgs);
	
The SDK functions using a "callback" and "subscription" design. To initialize the SDK call NodInitialize. This function accepts a function pointer to a function which accepts one argument, a NodEvent struct. The function pointer passed into the NodInitialize function will be the "callback" function which will recieve all data.

After calling NodInitialize the service and your application will start communicating with each other. Because the connection takes a short time to be established and NodInitialize is asynchronous, any attempt to use your Nod device before the connection is established will not work. Your "callback" function will alert you connection is established.

The "callback" function:

The callback function should take the form of: 

	void callbackFunction(NodEvent ev)

The NodEvent struct is defined in NodPlugin.h and is as follows:

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
        int sender = NOD_NONE;
    };
	
The type field refers to the type of event contained within the NodEvent struct the types are (also defined in NodPlugin.h):

    Button,
    Accelerometer,
    EulerAngles,
    AnalogData,
    Gestures,
    Pointer,
    Translation,
    Gyroscope,
    DeviceInfo,
    ServiceReady
	
For each event the NodEvent struct will only be populated with the information needed for that event. 

	Button -> buttonID, buttonState
	Accelerometer -> xf, yf, zf
	EulerAngles -> roll, pitch, yaw
	AnalogData -> x, y, trigger
	Gestures -> gesture
	Pointer -> x, y
	Translation -> x, y, z (Currently unsupported by firmware)
	Gyroscope -> xf, yf, zf
	DeviceInfo -> batteryPercent, firmwareVersion
	ServiceReady -> none
	
The "subscribe" function:

	NODPLUGIN_API bool NodSubscribe(Modality mode, const char* deviceName);
	
	This function requests data of a certain mode from a device. Modalities are defined in NodPlugin.h in the modality enum and are as follows:
	
	    ButtonMode
		AccelMode
		EulerMode
		GameControlMode
		GestureMode
		PointerMode
		TranslationMode
		GyroMode
		
	So NodSubscribe(EulerMode, NodGetRingName(0)) will subscribe to EulerAngles for the ring with id 0.
	
This is the bare information you need to get started with a Nod Device on Windows. In summary here are the steps to develop:

1. Create a "callback" function
2. call InitializeNod with your "callback" function
3. wait for your callback function to be called with a type of "ServiceReady"
4. Call NodNumRings() to determine the number of devices connected to your machine.
5. Call NodGetRingName on the id of your choice (the id is between 0 and NodNumrings) to retrieve the device's name.
6. Call Subscribe and pass in the modality and ring name of your choosing.
7. Your "callback" function should now be called whenever there is an update from your device.

This is a rough guide to the OpenSpatial Windows SDK for more information visit developer.nod.com or create a topic on our forums, https://forums.nod.com
	
 
	
	


