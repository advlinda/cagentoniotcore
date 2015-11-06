 
 libxml2 for Windows IoTCore
 ----------------------------------

 1. Invoke VS x86 Native Tools Command Prompt

 2. Set environment variables
	
	* You can either run IoT-Vars.cmd to setup environment variables,
     or do the step a and b as follows.
   
   (a) setup INCLUDE as VC++\Windows\Windows IoT Core

     > set INCLUDE=%VCINSTALLDIR%include;%VCINSTALLDIR%atlmfc\include;%WindowsSdkDir%Include\10.0.10240.0\ucrt;%WindowsSdkDir%Include\10.0.10240.0\um;%WindowsSdkDir%Include\10.0.10240.0\shared;%WindowsSdkDir%Include\10.0.10240.0\winrt;%NETFXSDKDir%Include\um
   
	   * Note: the above path can be found in the property of project of 
	   "Blank Windows IoT Core Console Application", and the target platform version 
	   of the project is 10.0.10240.0. You can find the paths by doing following things:
	   (i)  Open the Property page of the project: VC++ directories -> Include directories -> Edit
	   (ii) You shall found them in the Evaluated value box. If the target platform version is 
	        changed, the path should be changed as well.

    (b) setup LIB as VC++\Windows\Windows IoT Core
	
     > set LIB=%VCINSTALLDIR%lib\onecore;%WindowsSdkDir%lib\10.0.10240.0\um\x86;%NETFXSDKDir%lib\um\x86;%WindowsSdkDir%lib\10.0.10240.0\ucrt\x86
   
	   * Note: the above path can be found in the property of project of 
	   "Blank Windows IoT Core Console Application", and the target platform version 
	   of the project is 10.0.10240.0. You can find the paths by doing following things:
	   (i)  Open the Property page of the project: VC++ directories -> Library directories -> Edit
	   (ii) You shall found them in the Evaluated value box. If the target platform version is 
	        changed, the path should be changed as well.

 3. Navigate to directory libxml2-2.7.8\win32, and set up the configuration

   > Cscript configure.js prefix="<CAgent Project Directory>\WISEAgent\Library3rdParty\libxml2-2.7.8.win32\bin" include="<CAgent Project Directory>\WISEAgent\Library3rdParty\libxml2-2.7.8.win32\include" lib="<CAgent Project Directory>\WISEAgent\Library3rdParty\libxml2-2.7.8.win32\lib"

 4. Update file WISEAgent\Library3rdParty\libxml2-2.7.8\win32\Makefile.msvc

	§  Line 67 : Init LIBS to onecoreuap.lib
		LIBS = onecoreuap.lib

	§  Line 69 : use onecoreuap.lib
        LIBS = $(LIB)

	§  Line 84 : use onecoreuap.lib
        LIBS = $(LIB)

	§  Line 97 : remove “ /OPT:NOWIN98” to fix linking error
        LDFLAGS = $(LDFLAGS)

 5. Compile the source code 
   > nmake /f Makefile.msvc

 6. Binary files will be placed under libxml2-2.7.8\win32\bin.msvc

 7. If you want to build under Linux with the same source tree (maybe a shared directory for VM)

	> nmake /f Makefile.msvc distclean