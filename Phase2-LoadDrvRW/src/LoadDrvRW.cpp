//=====================================================================
// LoadDriver - Utility Application to load the Basic Driver
//
// This program dynamically loads the Basic driver. 
//
//  Last Modified : 7 March 2018 ronsto
//
//=====================================================================

#include <windows.h>
#include <stdio.h>

//=====================================================================
// main
//
// Main Entry Point
//=====================================================================

int __cdecl main(int argc, char* argv[]) {
    //Local Variables

    SC_HANDLE hSCM;
    SC_HANDLE hService;
    SERVICE_STATUS ss;
    int iBufferSize;
    DWORD dwgle;


    //Process Arguments

    if (argc < 2) {
        printf("LoadDrv requires the path to BasicDrv.sys as a parameter.\n");
        printf("Example Usage LoadDrv C:\\temp\\BasicDrv.sys\n\n");
        return 2;
    }

    printf("Load Driver: %s\n", argv[1]);

    //Open SCM

    printf("Opening SCM...\n");

    hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);

    if (!hSCM) {

        dwgle = GetLastError();
        printf("OpenSCManager encountered the error %d. Check winerror.h\n", dwgle);
        return 3;

    }

    //argv[1] contains the binary path in ANSI format. CreateService (param 8) requires
    //this to be converted to a Unicode (wide) string. MultiByteToWideChar is used for this.
    //First get the size of the required buffer for the wide string (first call
    //to MultiByteToWideChar)

    iBufferSize = MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, NULL, 0);

    //now we have the size of the buffer to allocate

    wchar_t* wstrServiceBinaryPath = new wchar_t[iBufferSize];

    //next use iBufferSize to specify the size of our string in the second call

    MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, wstrServiceBinaryPath, iBufferSize);

    //Add our service to SCM database

    printf("Creating Service named BasicDrv...\n");

    hService = CreateService(hSCM, // Handle to SCM database
        TEXT("BasicDrvRW"),          // Name of service
        TEXT("Basic Driver ReadWrite"),      // Display name
        SERVICE_ALL_ACCESS,        // Desired access
        SERVICE_KERNEL_DRIVER,     // Service type
        SERVICE_DEMAND_START,      // Start type
        SERVICE_ERROR_NORMAL,      // Error control type
        wstrServiceBinaryPath,     // Service binary
        NULL,                      // No load ordering group
        NULL,                      // No tag identifier
        NULL,                      // Dependencies
        NULL,                      // LocalSystem account
        NULL);                     // no password

    delete[] wstrServiceBinaryPath;

    if (!hService) {

        dwgle = GetLastError();
        DeleteService(hService);
        printf("CreateService encountered the error %d. Check winerror.h\n", dwgle);
        return 4;
    }



    //Open the service

    printf("Opening Service...\n");

    hService = OpenService(hSCM,    // Handle to SCM database
        TEXT("BasicDrvRW"),           // Name of service
        SERVICE_ALL_ACCESS);        // Desired access

    if (!hService) {

        dwgle = GetLastError();
        DeleteService(hService);
        printf("OpenService encountered the error %d. Check winerror.h\n", dwgle);
        return 5;
    }



    //Start the service

    printf("Starting Service...\n");

    if (!StartService(hService,   // Handle to service
        0,                        // Number of arguments
        NULL))                    // Arguments
    {
        dwgle = GetLastError();
        DeleteService(hService);
        CloseServiceHandle(hSCM);
        CloseServiceHandle(hService);
        printf("StartService encountered the error %d. Check winerror.h\n", dwgle);
        return 6;
    }

    printf("Made it here \n");

    //Wait For User

    printf("Press <ENTER> to stop service\r\n");
    getchar();


    //Stop the service

    ControlService(hService, SERVICE_CONTROL_STOP, &ss);


    //Cleanup
    DeleteService(hService);
    CloseServiceHandle(hSCM);
    CloseServiceHandle(hService);

    return 1;
}