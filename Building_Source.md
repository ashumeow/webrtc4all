

To build the source code you'll need Visual Studio 2008 or later.<br />
Please note that webrtc4all requires [Doubango v2.x](http://code.google.com/p/doubango/)

## Supported systems ##
  * Windows XP SP3
  * Windows Vista
  * Windows 7

## Thirdparties ##
  * Microsoft DirectX SDK: http://www.microsoft.com/downloads/en/details.aspx?FamilyID=c72d9f1e-53f3-4747-8490-6801d8e8b4ef
  * Windows Platform SDK: Depends on your system. Google is your friend :)
After the installation, you should add both the **lib** and **include** folders to the visual studio search paths like this:
  * Open "Tools -> Options -> Projects and Solutions -> VC++ Directories -> Include Files" and add **$(WindowsSdkDir)\include** and **$(DXSDK\_DIR)include**
  * Open "Tools -> Options -> Projects and Solutions -> VC++ Directories -> Library Files" and add **$(WindowsSdkDir)\lib** and **$(DXSDK\_DIR)lib\x86**
Please note that for old Platform SDK the varenv name is **PSDK\_DIR** instead of **WindowsSdkDir**

## Building the source code ##
  * checkout webrtc4all source code into **webrtc4all-read-only**
```
svn checkout http://webrtc4all.googlecode.com/svn/trunk/ webrtc4all-read-only
```
  * Checkout doubango source code into **webrtc4all-read-only/thirdparties/source**. Important: The destination directory MUST be called **doubango**
```
svn checkout http://doubango.googlecode.com/svn/branches/2.0/doubango doubango
```

<br />
To open & build all projects, open **webrtc4all-read-only\webrtc4all.sln**