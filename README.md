openspatial-windows
===================

Open Spatial Windows SDK

Open Spatial Service Instructions (v0.0.4)

Welcome to the Open Spatial Windows Service. Here are some basic instructions to get you started.

As of 9/21/2015 (v0.0.3) the Windows SDK and service has been majorly modified. We have rewritten the entire SDK and service to improve robustness. Please refer to this guide to learn how to use the new SDK


===================

For SDK Documentation please visit: http://dev.nod.com/docs/windows/getting-started/

NOTE: EXAMPLE APP INSTRUCTIONS HAVE BEEN CHANGED AND MAY NOT BE REFLECTED ON OUR DEVELOPER SITE

Example Application Instructions:

Note: The example app requires cmake v2.8.8 or higher. Please download from, https://cmake.org/download/

1. Install CMake and the Nod SDK
2. navigate to the OpenSpatialExample directory in a CMD prompt.
3. create a build folder (mkdir build)
4. cd into the build directory (cd build)
5. run: cmake .. -G "Visual Studio 14 2015"
6. This will generate a visual studio solution, which you should be able to build and run.

Note: to use different compilers (ninja, visual studio 2013, etc) you can change the -G argument please
refer to CMake documentation for the types of genrators.

It is not required to use cmake with the Nod SDK we have just created the example application with cmake for 
ease of distribution across different systems.
