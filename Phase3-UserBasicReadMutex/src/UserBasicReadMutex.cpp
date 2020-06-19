#include <conio.h>
#include <iostream>
#include <strsafe.h>
#include <windows.h>

#define MAXBUFFER 255

int main(void) {
    DWORD dwReturn;

    auto* mutex = OpenMutex(SYNCHRONIZE, false, L"BASICDRVMUTEX");

    if (!mutex) {
        std::cout << "Failed to open mutex: " << GetLastError() << std::endl;
        return 2;
    }

    const auto readString = std::make_unique<std::wstring>();

    if (!readString) {
        std::cout << "Failed to allocate memory\n";
        return 2;
    }

    // Open DOS Device Name
    auto* hFile = CreateFile(
        L"\\\\.\\BasicDrvMutex",    // Name of object
        GENERIC_READ,               // Desired Access
        0,                          // Share Mode
        nullptr,                    // reserved
        OPEN_EXISTING,              // Fail of object does not exist
        0,                          // Flags
        nullptr                     // reserved
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cout << "CreateFile failed to open handle to BasicRW Device Object\n";
        return 4;
    }

    auto count = 1;

    while (count < 100) {
        std::cout << "\n Waiting for BASICDRVMUTEX \n";
        Sleep(100);

        WaitForSingleObject(mutex, INFINITE);

        std::cout << "\n Reading string from driver buffer \n";

        ReadFile(hFile, readString.get(), MAXBUFFER, &dwReturn, nullptr);

        std::cout << "\n Results from buffer - " << readString << std::endl;
        Sleep(100);

        std::cout << "\n Release mutex \n";
        Sleep(100);

        ReleaseMutex(mutex);
        count++;
    }

    CloseHandle(mutex);
    CloseHandle(hFile);
}
