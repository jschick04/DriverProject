//=====================================================================
// UserBasic - User Basic Driver
//
// This program writes a message to the Basic Driver.
//
//   Last Modified : 4 Feb 2015 ronsto
//
//=====================================================================

#include <conio.h>
#include <stdio.h>
#include <strsafe.h>
#include <windows.h>

#define MAXBUFFER 255

//=====================================================================
// main
//
// Main Entry Point
//=====================================================================

int main(void) {
    //Local Variables

    HANDLE hFile;
    DWORD dwReturn;
    HANDLE hMutex = CreateMutex(nullptr, FALSE, L"BASICDRVMUTEX");

    if (!hMutex) {
        printf("Failed to create mutex");
        return 2;
    }

    //Used for the ReadFile and WriteFile operations
    BOOL fSuccess = FALSE;

    // Initial string to seed the buffer
    char szInitialString[] = "BasicDrvMutex Test Write Complete!\0";

    char* szWriteString = (char*)malloc(MAXBUFFER);

    if (!szWriteString) {
        printf("Failed to malloc");
        return 2;
    }

    const char* prepend = "Driver Buffer Write Number";

    //Open DOS Device Name 

    // @formatter:off
    hFile = CreateFile(
        L"\\\\.\\BasicDrvMutex",              // Name of object
        GENERIC_READ | GENERIC_WRITE,   // Desired Access
        0,                              // Share Mode
        NULL,                           // reserved
        OPEN_EXISTING,                  // Fail of object does not exist
        0,                              // Flags
        NULL                            // reserved
    );
    // @formatter:on

    if (hFile == INVALID_HANDLE_VALUE) {
        printf("CreateFile failed to open handle to BasicRW Device Object");
        return 4;
    }

    //Write Initial String To Driver

    // @formatter:off
    fSuccess = WriteFile(
        hFile,                      // File Handle
        szInitialString,            // Pointer to buffer
        sizeof(szInitialString),    // Number of bytes to write
        &dwReturn,                  // Number of bytes written
        nullptr                     // Not overlapped
    );
    // @formatter:on

    int count = 1;

    while (count < 100) {
        printf("\n Waiting for BASICDRVMUTEX \n");

        Sleep(100);
        WaitForSingleObject(hMutex, INFINITE);
        printf("\n BASICDRVMUTEX acquired \n");

        StringCchPrintfA(szWriteString, MAXBUFFER, "%s %d", prepend, count);

        printf("\n Writing the string to driver - %s \n", szWriteString);
        Sleep(100);

        WriteFile(hFile, szWriteString, strlen(szWriteString), &dwReturn, nullptr);

        printf("\n Release mutex \n");
        Sleep(100);

        ReleaseMutex(hMutex);
        count++;
    }

    CloseHandle(hMutex);
    CloseHandle(hFile);
    free(szWriteString);
}
