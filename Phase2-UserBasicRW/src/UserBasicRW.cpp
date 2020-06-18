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

    //Used for the ReadFile and WriteFile operations
    BOOL fSuccess = FALSE;

    char prompt = 0;

    // Initial string to seed the buffer
    char szInitialString[] = "BasicDrvRW Test Write Complete!\0";

    char* szWriteString = (char*)malloc(MAXBUFFER);

    if (!szWriteString) {
        printf("Failed to malloc");
        return 2;
    }

    char* szReadString = (char*)malloc(MAXBUFFER);

    if (!szReadString) {
        printf("Failed to malloc");
        return 3;
    }

    //Open DOS Device Name 

    hFile = CreateFile(
        L"\\\\.\\BasicRW",
        // Name of object
        GENERIC_READ | GENERIC_WRITE,
        // Desired Access
        0,
        // Share Mode
        NULL,
        // reserved
        OPEN_EXISTING,
        // Fail of object does not exist
        0,
        // Flags
        NULL
    ); // reserved

    if (hFile == INVALID_HANDLE_VALUE) {
        printf("CreateFile failed to open handle to BasicRW Device Object");
        return 4;
    }

    //Write Initial String To Driver 

    fSuccess = WriteFile(
        hFile,
        // File Handle
        szInitialString,
        // Pointer to buffer
        sizeof(szInitialString),
        // Number of bytes to write
        &dwReturn,
        // Number of bytes written
        NULL
    ); // Not overlapped

    while (1) {
        printf("Press 'w' to write to the driver or 'r' to read to the driver. Press 'q' to quit.\n\n");

        prompt = (char)_getch();

        if (('w' == prompt) || ('W' == prompt)) {
            printf("Please enter the string to write to BasicDrvRW:\n");

            if (fgets(szWriteString, MAXBUFFER, stdin) != 0) {
                printf("The string you entered is: %s\n", szWriteString);

                fSuccess = WriteFile(
                    hFile,
                    szWriteString,
                    strlen(szWriteString),
                    &dwReturn,
                    NULL
                );

                if (!fSuccess) {
                    printf("The WriteFile Operation failed\n\n");
                } else {
                    printf("The WriteFile Operation Succeeded\n\n");
                }
            } else {
                printf("The string you entered doesn't contain any characters. Skipping Write\n\n");
            }
        } else if (('r' == prompt) || ('R' == prompt)) {
            fSuccess = ReadFile(
                hFile,
                szReadString,
                MAXBUFFER,
                &dwReturn,
                NULL
            );

            if (!fSuccess) {
                printf("The ReadFile Operation failed\n\n");
            } else {
                printf("The string retrieved from the driver's buffer is: %s\n\n", szReadString);
            }
        } else if (('q' == prompt) || ('Q' == prompt)) {
            CloseHandle(hFile);
            return 1;
        } else {
            printf("Only 'w' and 'r' are valid operations. Press 'q' to quit.\n\n");
        }
    }
}
