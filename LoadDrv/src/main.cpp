#include <cstdio>
#include <iostream>
#include <windows.h>

int __cdecl main(int argc, char* argv[]) {
    SERVICE_STATUS ss;

    // Process Arguments
    if (argc < 2) {
        std::cout << "LoadDrv requires the path to BasicDrv.sys as a parameter.\n";
        std::cout << "Example Usage LoadDrv C:\\temp\\BasicDrv.sys\n\n";
        return 2;
    }

    std::cout << "Load Driver: " << argv[1] << std::endl;

    std::cout << "Opening SCM...\n";

    SC_HANDLE hSCM = OpenSCManager(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);

    if (!hSCM) {
        std::cout << "OpenSCManager encountered the error " << GetLastError() << ". Check winerror.h\n";

        return 3;
    }

    // argv[1] contains the binary path in ANSI format. CreateService (param 8) requires
    // this to be converted to a Unicode (wide) string. MultiByteToWideChar is used for this.
    // First get the size of the required buffer for the wide string (first call
    // to MultiByteToWideChar)
    const int iBufferSize = MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, nullptr, 0);

    auto* wstrServiceBinaryPath = new wchar_t[iBufferSize];

    MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, wstrServiceBinaryPath, iBufferSize);

    // Add our service to SCM database
    std::cout << "Creating Service named BasicDrv...\n";

    SC_HANDLE hService = CreateService(
        hSCM,                   // Handle to SCM database
        TEXT("BasicDrv"),       // Name of service
        TEXT("Basic Driver"),   // Display name
        SERVICE_ALL_ACCESS,     // Desired access
        SERVICE_KERNEL_DRIVER,  // Service type
        SERVICE_DEMAND_START,   // Start type
        SERVICE_ERROR_NORMAL,   // Error control type
        wstrServiceBinaryPath,  // Service binary
        nullptr,                // No load ordering group
        nullptr,                // No tag identifier
        nullptr,                // Dependencies
        nullptr,                // LocalSystem account
        nullptr                 // no password
    );

    delete[] wstrServiceBinaryPath;

    if (!hService) {
        std::cout << "CreateService encountered the error " << GetLastError() << ". Check winerror.h\n";

        DeleteService(hService);

        return 4;
    }

    std::cout << "Opening Service...\n";

    hService = OpenService(
        hSCM,               // Handle to SCM database
        TEXT("BasicDrv"),   // Name of service
        SERVICE_ALL_ACCESS  // Desired access
    );

    if (!hService) {
        std::cout << "OpenService encountered the error " << GetLastError() << ". Check winerror.h\n";

        DeleteService(hService);

        return 5;
    }

    std::cout << "Starting Service...\n";

    if (!StartService(hService, 0, nullptr)) {
        std::cout << "StartService encountered the error " << GetLastError() << ". Check winerror.h\n";

        DeleteService(hService);
        CloseServiceHandle(hSCM);
        CloseServiceHandle(hService);

        return 6;
    }

    std::cout << "Made it here \n";

    std::cout << "Press <ENTER> to stop service\r\n";
    getchar();

    // Stop the service
    ControlService(hService, SERVICE_CONTROL_STOP, &ss);

    DeleteService(hService);
    CloseServiceHandle(hSCM);
    CloseServiceHandle(hService);
}
