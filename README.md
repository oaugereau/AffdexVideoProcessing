#Sample Apps for Affdex SDK for Windows 

Welcome to our repository on GitHub! Here you will find example code to get you started with our Affdex SDK 2.0 for Windows and begin emotion-enabling you own app!  Documentation for the Windows SDK is at <a href=http://developer.affectiva.com/windows/>Affectiva's Developer Portal</a>.

OpenCV-webcam-demo
------------------

*Dependencies*

- Affdex SDK 2.0 (32 bit)
- OpenCV for Windows 2.4.9: http://sourceforge.net/projects/opencvlibrary/files/opencv-win/2.4.9/
- Visual Studio 2013 or higher

OpenCV-webcam-demo is a simple app that uses the camera connected to your PC to view your facial expressions and face points.

In order to use the project, you will need to:
- Contact Affectiva at sales@affectiva.com to obtain the SDK.
- Install the SDK using MSI installer.
- Download OpenCV package and extract it into ${SRC_ROOT}\opencv
- Use affdex-win-samples.sln to build the sample app


AffdexMe
--------

**AffdexMe** is a windows application that demonstrates the use of the Affdex SDK for Windows. It uses the camera on your Windows PC to view, process and analyze live video of your face. Start the app and you will see your own face on the screen, and metrics describing your expressions.

See AffdexMe/README.md for more information
