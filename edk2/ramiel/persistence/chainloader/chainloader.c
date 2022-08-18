#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDevicePathLib/UefiDevicePathLib.h>
#include <Library/BaseMemoryLib.h>

#define APPLICATION_SIZE_VAR L"application_size"
#define APPLICATION_SIZE_VAR_GUID {0x875A3D03, 0x6DBD, 0x4E20, {0xB0, 0x3F, 0x53, 0x5E, 0xE3, 0x14, 0xDE, 0xB8}}

#define APPLICATION_VAR L"application"
#define APPLICATION_VAR_GUID {0x2C299EB5, 0x7424, 0x4580, {0xA1 0xC7, 0x22, 0xFB, 0xDB, 0x8A, 0x71, 0x13}}

EFI_STATUS EFIAPI UefiEntry(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable) {
    gST = SystemTable;
    gBS = SystemTable->BootServices;
    gRT = SystemTable->RuntimeServices;
    gImageHandle = ImageHandle;
    gBS->SetWatchdogTimer(0, 0, 0, NULL);

    EFI_STATUS status;

    EFI_GUID application_size_var_guid = APPLICATION_SIZE_VAR_GUID;
    UINT64 application_size;
    UINT64 application_size_var_size = sizeof(UINT64);

    gRT->GetVariable(
        APPLICATION_SIZE_VAR,
        &application_size_var_guid,
        NULL,
        &application_size_var_size,
        &application_size);

    EFI_GUID application_var_guid = APPLICATION_VAR_GUID;
    void *application_ptr = AllocatePages(EFI_SIZE_TO_PAGES(application_size));
    UINT64 application_var_size = application_size;

    gRT->GetVariable(
        APPLICATION_VAR,
        &application_var_guid,
        NULL,
        &application_var_size,
        application_ptr);

    // https://stackoverflow.com/questions/31317566/uefi-loadimage-hangs
    MEMMAP_DEVICE_PATH mempath[2];
    mempath[0].Header.Type = HARDWARE_DEVICE_PATH;
    mempath[0].Header.SubType = HW_MEMMAP_DP;
    mempath[0].Header.Length[0] = (UINT8) sizeof(MEMMAP_DEVICE_PATH);
    mempath[0].Header.Length[1] = (UINT8) (sizeof(MEMMAP_DEVICE_PATH) >> 8);
    mempath[0].MemoryType = EfiLoaderCode;
    mempath[0].StartingAddress = (UINT64) buff_ptr;
    mempath[0].EndingAddress = (UINT64) ((UINT8 *) application_ptr + application_size);

    mempath[1].Header.Type = END_DEVICE_PATH_TYPE;
    mempath[1].Header.SubType = END_INSTANCE_DEVICE_PATH_SUBTYPE;
    mempath[1].Header.Length[0] = (UINT8) sizeof(EFI_DEVICE_PATH);
    mempath[1].Header.Length[1] = (UINT8) (sizeof(EFI_DEVICE_PATH) >> 8);

    EFI_HANDLE NewImageHandle;
    status = gBS->LoadImage(
        TRUE,
        gImageHandle,
        (EFI_DEVICE_PATH *) mempath,
        application_ptr,
        application_size,
        &NewImageHandle);
    if (EFI_ERROR(status)) {
        return status;
    }
    status = gBS->StartImage(NewImageHandle, NULL, NULL);
    if (EFI_ERROR(status)) {
        return status;
    }

    return status;
}
