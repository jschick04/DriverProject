#include <conio.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <strsafe.h>
#include <windows.h>

#define MAXBUFFER 255

int main(void) {
    DWORD dwReturn;

    auto* mutex = CreateMutex(nullptr, FALSE, L"BASICDRVMUTEX");

    if (!mutex) {
        std::cout << "Failed to create mutex: " << GetLastError() << std::endl;
        return 2;
    }

    // Initial string to seed the buffer
    const std::wstring szInitialString = L"BasicDrvMutex Test Write Complete!";

    //Open DOS Device Name 
    auto* hFile = CreateFile(
        L"\\\\.\\BasicDrvMutex",        // Name of object
        GENERIC_READ | GENERIC_WRITE,   // Desired Access
        0,                              // Share Mode
        nullptr,                        // reserved
        OPEN_EXISTING,                  // Fail of object does not exist
        0,                              // Flags
        nullptr                         // reserved
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        printf("CreateFile failed to open handle to BasicRW Device Object");
        return 4;
    }

    //Write Initial String To Driver
    const auto success = WriteFile(
        hFile,                      // File Handle
        szInitialString.c_str(),    // Pointer to buffer
        szInitialString.size(),     // Number of bytes to write
        &dwReturn,                  // Number of bytes written
        nullptr                     // Not overlapped
    );

    if (!success) {
        std::cout << "Failed to WriteFile with initial string\n";
        return 5;
    }

    int count = 1;

    while (count < 100) {
        std::cout << "\n Waiting for BASICDRVMUTEX \n";

        Sleep(100);
        WaitForSingleObject(mutex, INFINITE);
        std::cout << "\n BASICDRVMUTEX acquired \n";

        auto writeString = L"Driver Buffer Write Number " + std::to_wstring(count);

        std::cout << "\n Writing the string to driver - " << writeString.c_str() << std::endl;
        Sleep(100);

        WriteFile(hFile, writeString.c_str(), writeString.size(), &dwReturn, nullptr);

        std::cout << "\n Release mutex \n";
        Sleep(100);

        ReleaseMutex(mutex);
        count++;
    }

    CloseHandle(mutex);
    CloseHandle(hFile);
}
