#include <ntddk.h>

#include "drvioctl.h"

#include "trace.h"

#include "BasicDrv.tmh"

constexpr auto NT_DEVICE_NAME = L"\\Device\\BasicDrv";
constexpr auto DOS_DEVICE_NAME = L"\\DosDevices\\BasicDrv";

constexpr auto MAXDATABUFFER = 255;

void* g_dataBuffer = nullptr;
PERESOURCE g_bufferLock = nullptr;

#pragma region CircularArrayDebugging

#define SCI_CIRCULAR_ENTRY_COUNT 10000
#define SCI_KERNEL_STACK_FRAMES 16

typedef struct StateChangeInfo {
    PETHREAD thread;
    PEPROCESS process;
    long dwTickCount;
    ULONG IOCTL_Captured;
    int nFramesKernel;
    PVOID traceKernel[SCI_KERNEL_STACK_FRAMES];
} STATECHANGEINFO, *PSTATECHANGEINFO;

STATECHANGEINFO g_sciCircularBuffer[SCI_CIRCULAR_ENTRY_COUNT];

long g_dwSCIIndex = -1;

void CaptureStateChange(ULONG IOCTL_Sent) {
    LONG currentIndex = 0;
    PSTATECHANGEINFO pSCI;

    currentIndex = InterlockedIncrement(&g_dwSCIIndex);

    pSCI = &(g_sciCircularBuffer[currentIndex % SCI_CIRCULAR_ENTRY_COUNT]);

    pSCI->thread = PsGetCurrentThread();
    pSCI->process = PsGetCurrentProcess();
    pSCI->IOCTL_Captured = IOCTL_Sent;

    pSCI->nFramesKernel = RtlWalkFrameChain(pSCI->traceKernel, SCI_KERNEL_STACK_FRAMES, 0);

    if (pSCI->nFramesKernel < SCI_KERNEL_STACK_FRAMES) {
        pSCI->nFramesKernel += RtlWalkFrameChain(
            pSCI->traceKernel + pSCI->nFramesKernel,
            SCI_KERNEL_STACK_FRAMES - pSCI->nFramesKernel,
            1
        );
    }
}

#pragma endregion

#pragma region Definitions

extern "C" {

    NTSTATUS DriverEntry(
        IN OUT PDRIVER_OBJECT pDriverObject,
        IN PUNICODE_STRING pRegistryPath
    );

    void BasicUnloadDriver(IN PDRIVER_OBJECT pDriverObject);

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

    NTSTATUS MyDriverDispatchDeviceControl(
        IN PDEVICE_OBJECT pDeviceObject,
        IN PIRP pIrp
    );

}

#pragma endregion

/*Compiler Directives
* These compiler directives tell the OS
* how to load the driver into memory. The WDK explains these in details
* INIT is for onetime initialization code that can be permanently discarded after the driver is loaded.
* PAGE code must run at passive and not raise IRQL >= dispatch.*/

#pragma alloc_text(INIT, DriverEntry )
#pragma alloc_text(PAGE, BasicUnloadDriver)
#pragma alloc_text(PAGE, BasicCreateClose)
#pragma alloc_text(PAGE, BasicWrite)
#pragma alloc_text(PAGE, BasicRead)
#pragma alloc_text(PAGE, MyDriverDispatchDeviceControl)

NTSTATUS DriverEntry(IN OUT PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath) {
    PDEVICE_OBJECT pDeviceObject = nullptr; // Pointer to our new device object
    UNICODE_STRING usDeviceName;            // Device Name
    UNICODE_STRING usDosDeviceName;         // DOS Device Name

    //DbgPrint("DriverEntry Called\r\n");
    WPP_INIT_TRACING(pDriverObject, pRegistryPath);

    RtlInitUnicodeString(&usDeviceName, NT_DEVICE_NAME);
    RtlInitUnicodeString(&usDosDeviceName, DOS_DEVICE_NAME);

    g_bufferLock = static_cast<PERESOURCE>(ExAllocatePoolWithTag(NonPagedPool, sizeof(ERESOURCE), '2saB'));
    NTSTATUS ntStatus = ExInitializeResourceLite(g_bufferLock);

    if (!NT_SUCCESS(ntStatus)) {
        DbgPrint("Could not create the device object\n");
        WPP_CLEANUP(pDriverObject);

        return ntStatus;
    }

    ntStatus = IoCreateDevice(
        pDriverObject,              // Driver Object
        0,                          // No device extension
        &usDeviceName,              // Device name "\Device\Basic"
        FILE_DEVICE_UNKNOWN,        // Device type
        FILE_DEVICE_SECURE_OPEN,    // Device characteristics
        false,                      // Not an exclusive device
        &pDeviceObject              // Returned pointer to Device Object
    );

    if (!NT_SUCCESS(ntStatus)) {
        DbgPrint("Could not create the device object\n");
        WPP_CLEANUP(pDriverObject);

        return ntStatus;
    }

    // Initialize the entry points in the driver object 
    pDriverObject->MajorFunction[IRP_MJ_CREATE] = BasicCreateClose;
    pDriverObject->MajorFunction[IRP_MJ_CLOSE] = BasicCreateClose;
    pDriverObject->MajorFunction[IRP_MJ_WRITE] = BasicWrite;
    pDriverObject->MajorFunction[IRP_MJ_READ] = BasicRead;
    pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MyDriverDispatchDeviceControl;

    pDriverObject->DriverUnload = BasicUnloadDriver;

    // With Direct IO the MdlAddress describes the Virtual Address 
    // list. This locks pages in memory. Pages are not copied.     

    pDeviceObject->Flags |= DO_DIRECT_IO;

    ntStatus = IoCreateSymbolicLink(&usDosDeviceName, &usDeviceName);

    if (!NT_SUCCESS(ntStatus)) {
        DbgPrint("Could not create the symbolic link\n");
        IoDeleteDevice(pDeviceObject);
    }

    if (!g_dataBuffer) {
        g_dataBuffer = ExAllocatePoolWithTag(PagedPool, MAXDATABUFFER, static_cast<LONG>('ISAB'));

        if (!g_dataBuffer) {
            DbgPrint("Failed to allocate pool.");
        }
    }

    return ntStatus;
}

void BasicUnloadDriver(IN DRIVER_OBJECT* const pDriverObject) {
    UNICODE_STRING usDeviceName;    // Device Name
    UNICODE_STRING usDosDeviceName; // DOS Device Name

    //DbgPrint("BasicUnloadDriver Called\r\n");
    WPP_CLEANUP(pDriverObject);

    RtlInitUnicodeString(&usDeviceName, NT_DEVICE_NAME);
    RtlInitUnicodeString(&usDosDeviceName, DOS_DEVICE_NAME);

    IoDeleteSymbolicLink(&usDosDeviceName);

    DEVICE_OBJECT* const pDeviceObject = pDriverObject->DeviceObject;

    if (pDeviceObject != nullptr) {
        IoDeleteDevice(pDeviceObject);
    }

    if (g_dataBuffer) {
        ExFreePoolWithTag(g_dataBuffer, static_cast<LONG>('ISAB'));
    }

    if (g_bufferLock) {
        ExDeleteResourceLite(g_bufferLock);
        ExFreePoolWithTag(g_bufferLock, static_cast<LONG>('2saB'));
    }
}

NTSTATUS BasicCreateClose(IN PDEVICE_OBJECT /*pDeviceObject*/, IN IRP* const pIrp) {
    DbgPrint("BasicCreateClose Called\r\n");

    DoTraceMessage(LEVEL_INFO, "\nEntering %!FUNC!");

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS BasicWrite(IN PDEVICE_OBJECT /*pDeviceObject*/, IN IRP* const pIrp) {
    DbgPrint("BasicWrite Called\r\n");

    DoTraceMessage(LEVEL_INFO, "\nEntering %!FUNC!");

    IO_STACK_LOCATION* const pIoStackIrp = IoGetCurrentIrpStackLocation(pIrp);

    // Get Buffer Using MdlAddress Parameter From IRP
    if (pIoStackIrp) {
        void* pWriteDataBuffer = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority);

        KeEnterCriticalRegion();
        ExAcquireResourceExclusiveLite(g_bufferLock, true);

        if (pWriteDataBuffer) {
            // Verify That String Is NULL Terminated
            const ULONG uiLength = pIoStackIrp->Parameters.Write.Length;

            if (g_dataBuffer) {
                // First, clear the buffer from the last write
                RtlZeroMemory(g_dataBuffer, MAXDATABUFFER);

                // Copy the user buffer into our buffer
                RtlCopyMemory(g_dataBuffer, pWriteDataBuffer, uiLength);
            }
        }

        ExReleaseResourceLite(g_bufferLock);
        KeLeaveCriticalRegion();
    }

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS BasicRead(IN PDEVICE_OBJECT /*pDeviceObject*/, IN IRP* const pIrp) {
    DbgPrint("BasicRead Called\r\n");

    DoTraceMessage(LEVEL_INFO, "\nEntering %!FUNC!");

    IO_STACK_LOCATION* const pIoStackIrp = IoGetCurrentIrpStackLocation(pIrp);

    // Get Buffer Using MdlAddress Parameter from IRP
    if (pIoStackIrp) {
        void* pReadDataBuffer = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority);

        KeEnterCriticalRegion();
        ExAcquireResourceSharedLite(g_bufferLock, true);

        if (pReadDataBuffer) {
            // Verify That String Is NULL Terminated
            const unsigned int uiLength = pIoStackIrp->Parameters.Read.Length;

            if (g_dataBuffer) {
                RtlCopyMemory(pReadDataBuffer, g_dataBuffer, uiLength);
            }
        }

        ExReleaseResourceLite(g_bufferLock);
        KeLeaveCriticalRegion();
    }

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS MyDriverDispatchDeviceControl(IN PDEVICE_OBJECT /*pDeviceObject*/, IN IRP* const pIrp) {
    DbgPrint("DispatchDeviceControl Called\r\n");

    DoTraceMessage(LEVEL_INFO, "\nEntering %!FUNC!");

    IO_STACK_LOCATION* const pIoStackIrp = IoGetCurrentIrpStackLocation(pIrp);

    const ULONG controlCode = pIoStackIrp->Parameters.DeviceIoControl.IoControlCode;

    switch (controlCode) {
        case IOCTL_DEVICE_POWER_UP_EVENT :
            DbgPrint("Device Powering On\n");
            DbgPrint("..................\n");
            DbgPrint("Device is powered on and ready!\n");

            CaptureStateChange(IOCTL_DEVICE_POWER_UP_EVENT);

            break;
        case IOCTL_DEVICE_POWER_DOWN_EVENT :
            DbgPrint("Device Powering Down\n");
            DbgPrint("....................\n");
            DbgPrint("Goodbye for now.\n");

            CaptureStateChange(IOCTL_DEVICE_POWER_DOWN_EVENT);

            break;
        default :
            DbgPrint("Invalid Control Code");
            break;
    }

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}
