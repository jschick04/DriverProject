//=====================================================================
// BasicDrv - Basic Driver 
//
// This driver demonstrates writing to a kernel buffer 
//
//  Modified : 7 March 2018 ronsto
//  Last Modified: 9 January 2019 ronsto
//
//=====================================================================

//Include Files

#include <ntddk.h>

//Constants

#define NT_DEVICE_NAME      L"\\Device\\BasicDrvMutex"
#define DOS_DEVICE_NAME     L"\\DosDevices\\BasicDrvMutex"

// We are sending down a buff of 255 from the app
#define MAXDATABUFFER 255

PCHAR gDataBuffer = 0;

//Prototypes

NTSTATUS DriverEntry(
    IN OUT PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING pRegistryPath
);

VOID BasicUnloadDriver(IN PDRIVER_OBJECT pDriverObject);

//Called when a Create or Close IRP is sent to the driver
NTSTATUS BasicCreateClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
);

NTSTATUS BasicWrite(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
);

NTSTATUS BasicRead(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
);

/*Compiler Directives
* These compiler directives tell the OS
* how to load the driver into memory. The WDK explains these in details
* INIT is for onetime initialization code that can be permanently discarded after the driver is loaded.
* PAGE code must run at passive and not raise IRQL >= dispatch.*/

#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( PAGE, BasicUnloadDriver)
#pragma alloc_text( PAGE, BasicCreateClose)
#pragma alloc_text( PAGE, BasicWrite)
#pragma alloc_text( PAGE, BasicRead)

//=====================================================================
// DriverEntry
//
// This routine is called by the Operating System to initialize 
// the driver. It creates the device object, fills in the dispatch 
// entry points and completes the initialization.
//
// Returns STATUS_SUCCESS if initialized; an error otherwise.
//=====================================================================

NTSTATUS DriverEntry(IN OUT PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath) {
    UNREFERENCED_PARAMETER(pRegistryPath);
    //Locals

    NTSTATUS ntStatus;

    PDEVICE_OBJECT pDeviceObject = NULL; // Pointer to our new device object
    UNICODE_STRING usDeviceName; // Device Name
    UNICODE_STRING usDosDeviceName; // DOS Device Name

    DbgPrint("DriverEntry Called\r\n");

    //Initialize Unicode Strings

    RtlInitUnicodeString(&usDeviceName, NT_DEVICE_NAME);
    RtlInitUnicodeString(&usDosDeviceName, DOS_DEVICE_NAME);

    //Create Device Object

    ntStatus = IoCreateDevice(
        pDriverObject,
        // Driver Object
        0,
        // No device extension
        &usDeviceName,
        // Device name "\Device\Basic"
        FILE_DEVICE_UNKNOWN,
        // Device type
        FILE_DEVICE_SECURE_OPEN,
        // Device characteristics
        FALSE,
        // Not an exclusive device
        &pDeviceObject
    ); // Returned pointer to Device Object

    if (!NT_SUCCESS(ntStatus)) {
        DbgPrint("Could not create the device object\n");
        return ntStatus;
    }

    //Initialize the entry points in the driver object 

    pDriverObject->MajorFunction[IRP_MJ_CREATE] = BasicCreateClose;
    pDriverObject->MajorFunction[IRP_MJ_CLOSE] = BasicCreateClose;
    pDriverObject->MajorFunction[IRP_MJ_WRITE] = BasicWrite;
    pDriverObject->MajorFunction[IRP_MJ_READ] = BasicRead;

    pDriverObject->DriverUnload = BasicUnloadDriver;

    //Set Flags

    //With Direct IO the MdlAddress describes the Virtual Address 
    //list. This locks pages in memory. Pages are not copied.     

    pDeviceObject->Flags |= DO_DIRECT_IO;

    //Clearing the following flag is not really necessary
    //because the IO Manager will clear it automatically.

    pDeviceObject->Flags &= (~DO_DEVICE_INITIALIZING);

    //Create a symbolic link 

    ntStatus = IoCreateSymbolicLink(&usDosDeviceName, &usDeviceName);

    if (!NT_SUCCESS(ntStatus)) {
        DbgPrint("Could not create the symbolic link\n");
        IoDeleteDevice(pDeviceObject);
    }

    // Create our kernel buffer

    if (!gDataBuffer) {
        // allocate a buffer with the tag BASI

        gDataBuffer = ExAllocatePoolWithTag(PagedPool, MAXDATABUFFER, (LONG)'ISAB');

        if (!gDataBuffer) {
            DbgPrint("Failed to allocate pool.");
        }
    }

    return ntStatus;
}

//=====================================================================
// BasicUnloadDriver
//
// This routine is called by the IO system to unload the driver. Any 
// resources previously allocated must be freed.
//=====================================================================

VOID BasicUnloadDriver(IN PDRIVER_OBJECT pDriverObject) {
    //Locals

    PDEVICE_OBJECT pDeviceObject = NULL; // Pointer to device object
    UNICODE_STRING usDeviceName; // Device Name
    UNICODE_STRING usDosDeviceName; // DOS Device Name

    DbgPrint("BasicUnloadDriver Called\r\n");

    //Initialize Unicode Strings for the IoDeleteSymbolicLink call

    RtlInitUnicodeString(&usDeviceName, NT_DEVICE_NAME);
    RtlInitUnicodeString(&usDosDeviceName, DOS_DEVICE_NAME);

    //Delete Symbolic Link

    IoDeleteSymbolicLink(&usDosDeviceName);

    //Delete Device Object

    pDeviceObject = pDriverObject->DeviceObject;

    if (pDeviceObject != NULL) {
        IoDeleteDevice(pDeviceObject);
    }

    if (gDataBuffer) {
        ExFreePoolWithTag(gDataBuffer, (LONG)'ISAB');
    }
}

//=====================================================================
// BasicCreateClose
//
// This routine is called by the IO system when the Basic device is 
// opened  (CreateFile) or closed (CloseHandle). No action is performed 
// other than completing the request successfully.
//=====================================================================

NTSTATUS BasicCreateClose(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {
    UNREFERENCED_PARAMETER(pDeviceObject);

    DbgPrint("BasicCreateClose Called\r\n");

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

//=====================================================================
// BasicWrite
//
// This routine is called when a write (WriteFile/WriteFileEx) is 
// issued on the device handle. This version uses Direct I/O.
//=====================================================================

NTSTATUS BasicWrite(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {
    UNREFERENCED_PARAMETER(pDeviceObject);

    //Locals

    unsigned int uiLength;

    PIO_STACK_LOCATION pIoStackIrp = NULL;
    PCHAR pWriteDataBuffer;

    DbgPrint("BasicWrite Called\r\n");

    //Retrieve Pointer To Current IRP Stack Location    

    pIoStackIrp = IoGetCurrentIrpStackLocation(pIrp);

    //Get Buffer Using MdlAddress Parameter From IRP

    if (pIoStackIrp) {
        pWriteDataBuffer = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority);

        if (pWriteDataBuffer) {
            //Verify That String Is NULL Terminated

            //uiIndex = 0;
            uiLength = pIoStackIrp->Parameters.Write.Length;

            // Copy string to the buffer

            if (gDataBuffer) {
                // First, clear the buffer from the last write
                RtlZeroMemory(gDataBuffer, MAXDATABUFFER);

                // Copy the user buffer into our buffer
                RtlCopyMemory(gDataBuffer, pWriteDataBuffer, uiLength);
            }
        }
    }

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS BasicRead(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp) {
    UNREFERENCED_PARAMETER(pDeviceObject);
    // Locals

    unsigned int uiIndex;
    unsigned int uiLength;

    PCHAR pReadDataBuffer;

    PIO_STACK_LOCATION pIoStackIrp = NULL;

    DbgPrint("BasicRead Called\r\n");

    // Retrieve Pointer to Current IRP Stack Location

    pIoStackIrp = IoGetCurrentIrpStackLocation(pIrp);

    // Get Buffer Using MdlAddress Parameter from IRP

    if (pIoStackIrp) {
        pReadDataBuffer = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority);

        if (pReadDataBuffer) {
            // Verify That String Is NULL Terminated

            uiIndex = 0;
            uiLength = pIoStackIrp->Parameters.Read.Length;

            if (gDataBuffer) {
                RtlCopyMemory(pReadDataBuffer, gDataBuffer, uiLength);
            }
        }
    }

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}
