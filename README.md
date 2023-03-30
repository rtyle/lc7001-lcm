LCM
========================

This repo contains the software for the Lighting Control Module.  The source code may be distributed as described in the LICENSE file.

Release Notes
---------------------------------
* [Release Notes](Doc/ReleaseNotes.md)

API Documentation
----------------------------------
* [LCM ICD](Doc/LCM_ICD.docx)

Setup Instructions
------------------------
* [Setup Virtual Machine](Doc/SetupVirtualMachine.md)
* [Install Kinetis Design Studio](Doc/InstallKDS.md) (Only required to load and execute the embedded firmware)

Build Instructions
------------------------
* [Build using cmake](Doc/BuildUsingCmake.md)

Run Server Application
------------------------
* Navigate to the Server build directory. 
    * <code>cd LCM/.build_amd/Server/LCMServer</code>
* Run the application
    * <code>./LCMServer</code>

Run Embedded Application
------------------------
* [Debug in Kinetis Design Studio](Doc/DebugInKDS.md)

Optional Build Tools
---------------------------------------
* [Optional Build Tools](Doc/Optional.md)
