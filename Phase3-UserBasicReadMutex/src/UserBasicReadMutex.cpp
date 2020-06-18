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
    HANDLE hMutex = OpenMutex(SYNCHRONIZE, false, L"BASICDRVMUTEX");

    if (!hMutex) {
        printf("Failed to open mutex");
        return 2;
    }

    //Used for the ReadFile and WriteFile operations
    BOOL fSuccess = FALSE;

    char* szReadString = (char*)malloc(MAXBUFFER);

    if (!szReadString) {
        printf("Failed to malloc");
        return 2;
    }

    //Open DOS Device Name 

    // @formatter:off
    hFile = CreateFile(
        L"\\\\.\\BasicDrvMutex",              // Name of object
        GENERIC_READ,   // Desired Access
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

    int count = 1;

    while (count < 100) {
        printf("\n Waiting for BASICDRVMUTEX \n");

        Sleep(100);
        WaitForSingleObject(hMutex, INFINITE);
        printf("\n Reading string from driver buffer \n");

        ReadFile(hFile, szReadString, MAXBUFFER, &dwReturn, nullptr);

        printf("\n Results from buffer - %s \n", szReadString);
        Sleep(100);

        printf("\n Release mutex \n");
        Sleep(100);

        ReleaseMutex(hMutex);
        count++;
    }

    CloseHandle(hMutex);
    CloseHandle(hFile);
    free(szReadString);
}
