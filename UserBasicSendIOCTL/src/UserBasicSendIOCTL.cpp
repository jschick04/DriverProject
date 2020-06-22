#include "drvioctl.h"

#include <conio.h>
#include <iostream>
#include <string>
#include <strsafe.h>
#include <windows.h>

int main(void) {
    DWORD dwReturn;
    char prompt = 0;

    //Open DOS Device Name 
    HANDLE hFile = CreateFile(
        L"\\\\.\\BasicDrv",             // Name of object
        GENERIC_READ | GENERIC_WRITE,   // Desired Access
        0,                              // Share Mode
        nullptr,                        // reserved
        OPEN_EXISTING,                  // Fail of object does not exist
        0,                              // Flags
        nullptr                         // reserved
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cout << "CreateFile failed to open handle to Basic Device Object\n";
        return 4;
    }

    while (true) {
        std::cout << "Press (p) to power up, (s) for standby, (q) to quit.\n\n";

        std::cin >> prompt;

        if (('p' == prompt) || ('P' == prompt)) {
            DeviceIoControl(hFile, IOCTL_DEVICE_POWER_UP_EVENT, nullptr, 0, nullptr, 0, &dwReturn, nullptr);
        } else if (('s' == prompt) || ('S' == prompt)) {
            DeviceIoControl(hFile, IOCTL_DEVICE_POWER_DOWN_EVENT, nullptr, 0, nullptr, 0, &dwReturn, nullptr);
        } else if (('q' == prompt) || ('Q' == prompt)) {
            CloseHandle(hFile);

            return 1;
        } else {
            printf("Only 'w' and 'r' are valid operations. Press 'q' to quit.\n\n");

        }
    }
}
