#include <conio.h>
#include <iostream>
#include <string>
#include <strsafe.h>
#include <windows.h>

int main(void) {
    DWORD dwReturn;

    // Initial string to seed the buffer
    const std::string szInitialString = "BasicDrv Test Write Complete!";

    //Open DOS Device Name 
    auto* hFile = CreateFile(
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

    auto count = 1;

    while (count < 100) {
        Sleep(100);

        auto writeString = "Driver Buffer Write Number " + std::to_string(count);

        std::cout << "\n Writing the string to driver - " << writeString << std::endl;
        Sleep(100);

        WriteFile(hFile, writeString.c_str(), writeString.size(), &dwReturn, nullptr);

        Sleep(100);

        count++;
    }

    CloseHandle(hFile);
}
