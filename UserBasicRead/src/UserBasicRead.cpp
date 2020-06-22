#include <conio.h>
#include <iostream>
#include <strsafe.h>
#include <windows.h>

constexpr auto MAXBUFFER = 255;

int main(void) {
    DWORD dwReturn;

    char readString[MAXBUFFER] = {0};

    // Open DOS Device Name
    auto* hFile = CreateFile(
        L"\\\\.\\BasicDrv", // Name of object
        GENERIC_READ,       // Desired Access
        0,                  // Share Mode
        nullptr,            // reserved
        OPEN_EXISTING,      // Fail of object does not exist
        0,                  // Flags
        nullptr             // reserved
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cout << "CreateFile failed to open handle to Basic Device Object\n";
        return 4;
    }

    auto count = 1;

    while (count < 100) {
        Sleep(100);

        std::cout << "\n Reading string from driver buffer \n";

        ReadFile(hFile, readString, MAXBUFFER, &dwReturn, nullptr);

        std::cout << "\n Results from buffer - " << readString << std::endl;
        Sleep(100);

        count++;
    }

    CloseHandle(hFile);
}
