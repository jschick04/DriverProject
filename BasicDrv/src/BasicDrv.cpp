#include <ntddk.h>

#define NT_DEVICE_NAME      L"\\Device\\BasicDrv"
#define DOS_DEVICE_NAME     L"\\DosDevices\\BasicDrv"

#define MAXDATABUFFER 255

void* g_dataBuffer = nullptr;
PERESOURCE g_bufferLock = nullptr;

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

}

#pragma endregion

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

NTSTATUS DriverEntry(IN OUT PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING /*pRegistryPath*/) {
    PDEVICE_OBJECT pDeviceObject = nullptr; // Pointer to our new device object
    UNICODE_STRING usDeviceName;            // Device Name
    UNICODE_STRING usDosDeviceName;         // DOS Device Name

    DbgPrint("DriverEntry Called\r\n");

    RtlInitUnicodeString(&usDeviceName, NT_DEVICE_NAME);
    RtlInitUnicodeString(&usDosDeviceName, DOS_DEVICE_NAME);

    g_bufferLock = (PERESOURCE)ExAllocatePoolWithTag(NonPagedPool, sizeof(ERESOURCE), '2saB');
    auto ntStatus = ExInitializeResourceLite(g_bufferLock);

    if (!NT_SUCCESS(ntStatus)) {
        DbgPrint("Could not create the device object\n");
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
        return ntStatus;
    }

    // Initialize the entry points in the driver object 
    pDriverObject->MajorFunction[IRP_MJ_CREATE] = BasicCreateClose;
    pDriverObject->MajorFunction[IRP_MJ_CLOSE] = BasicCreateClose;
    pDriverObject->MajorFunction[IRP_MJ_WRITE] = BasicWrite;
    pDriverObject->MajorFunction[IRP_MJ_READ] = BasicRead;

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
        g_dataBuffer = ExAllocatePoolWithTag(PagedPool, MAXDATABUFFER, (LONG)'ISAB');

        if (!g_dataBuffer) {
            DbgPrint("Failed to allocate pool.");
        }
    }

    return ntStatus;
}

void BasicUnloadDriver(IN PDRIVER_OBJECT pDriverObject) {
    UNICODE_STRING usDeviceName;    // Device Name
    UNICODE_STRING usDosDeviceName; // DOS Device Name

    DbgPrint("BasicUnloadDriver Called\r\n");

    RtlInitUnicodeString(&usDeviceName, NT_DEVICE_NAME);
    RtlInitUnicodeString(&usDosDeviceName, DOS_DEVICE_NAME);

    IoDeleteSymbolicLink(&usDosDeviceName);

    auto* const pDeviceObject = pDriverObject->DeviceObject;

    if (pDeviceObject != nullptr) {
        IoDeleteDevice(pDeviceObject);
    }

    if (g_dataBuffer) {
        ExFreePoolWithTag(g_dataBuffer, (LONG)'ISAB');
    }

    if (g_bufferLock) {
        ExDeleteResourceLite(g_bufferLock);
        ExFreePoolWithTag(g_bufferLock, (LONG)'2saB');
    }
}

NTSTATUS BasicCreateClose(IN PDEVICE_OBJECT /*pDeviceObject*/, IN PIRP pIrp) {
    DbgPrint("BasicCreateClose Called\r\n");

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS BasicWrite(IN PDEVICE_OBJECT /*pDeviceObject*/, IN PIRP pIrp) {
    DbgPrint("BasicWrite Called\r\n");

    const auto* pIoStackIrp = IoGetCurrentIrpStackLocation(pIrp);

    // Get Buffer Using MdlAddress Parameter From IRP
    if (pIoStackIrp) {
        const auto* pWriteDataBuffer = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority);

        KeEnterCriticalRegion();
        ExAcquireResourceExclusiveLite(g_bufferLock, true);

        if (pWriteDataBuffer) {
            // Verify That String Is NULL Terminated
            const unsigned int uiLength = pIoStackIrp->Parameters.Write.Length;

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

NTSTATUS BasicRead(IN PDEVICE_OBJECT /*pDeviceObject*/, IN PIRP pIrp) {
    DbgPrint("BasicRead Called\r\n");

    const auto* pIoStackIrp = IoGetCurrentIrpStackLocation(pIrp);

    // Get Buffer Using MdlAddress Parameter from IRP
    if (pIoStackIrp) {
        auto* pReadDataBuffer = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority);

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
