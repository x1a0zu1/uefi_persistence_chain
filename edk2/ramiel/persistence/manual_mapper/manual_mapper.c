#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDevicePathLib/UefiDevicePathLib.h>
#include <Library/BaseMemoryLib.h>
#include <PeImage.h>

#define APPLICATION_SIZE_VAR L"application_size"
#define APPLICATION_SIZE_VAR_GUID {0x875A3D03, 0x6DBD, 0x4E20, {0xB0, 0x3F, 0x53, 0x5E, 0xE3, 0x14, 0xDE, 0xB8}}

#define APPLICATION_VAR L"application"
#define APPLICATION_VAR_GUID {0x2C299EB5, 0x7424, 0x4580, {0xA1 0xC7, 0x22, 0xFB, 0xDB, 0x8A, 0x71, 0x13}}

inline UINT64 EFIAPI RVA_to_VA(UINT64 ImageBase, UINT64 RVA) {
    return ImageBase + RVA;
}

EFI_STATUS EFIAPI load_image_from_mem(void *application_ptr, EFI_HANDLE *new_image_handle) {
    // adapted from: https://bidouillesecurity.com/tutorial-writing-a-pe-packer-part-2/
    EFI_STATUS status;

    EFI_IMAGE_DOS_HEADER *p_DOS_HDR = (EFI_IMAGE_DOS_HEADER *) application_ptr;
    EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION p_NT_HDR = (EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION *) (((char*) DosHdr) + DosHdr->e_lfanew);
    UINT64 hdr_image_base = p_NT_HDR->OptionalHeader.ImageBase;
    UINT64 size_of_image = p_NT_HDR->OptionalHeader.SizeOfImage;
    UINT64 size_of_headers = p_NT_HDR->OptionalHeader.SizeOfHeaders;

    UINT8 *ImageBase;

    status = gBS->AllocatePages(
        AllocateAnyPages,
        EfiLoaderCode,
        EFI_SIZE_TO_PAGES(size_of_image)
        ImageBase);
    if (EFI_ERROR(status)) {
        return status;
    }

    CopyMem(ImageBase, application_ptr, size_of_headers);
    EFI_IMAGE_SECTION_HEADER* sections = (EFI_IMAGE_SECTION_HEADER *) (p_NT_HDR + 1);

    for (int i = 0; i < p_NT_HDR->FileHeader.NumberOfSections; i++) {
        UINT8 *dest = RVA_to_VA(ImageBase, sections[i].VirtualAddress);

        if(sections[i].SizeOfRawData > 0) {
            CopyMem(dest, application_ptr + sections[i].PointerToRawData, sections[i].SizeOfRawData);
        }
        else {
            SetMem(dest, 0, sections[i].Misc.VirtualSize);
        }
    }

    EFI_IMAGE_DATA_DIRECTORY data_directory = p_NT_HDR->OptionalHeader.DataDirectory;

    UINT64 delta_VA_reloc = (UINT64) ImageBase - hdr_image_base;
    if (data_directory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress != 0 && delta_VA_reloc != 0) {
        EFI_IMAGE_BASE_RELOCATION *p_reloc = (EFI_IMAGE_BASE_RELOCATION *) RVA_to_VA(ImageBase, data_directory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
        while (p_reloc->VirtualAddress != 0) {
            UINT32 size = (p_reloc->SizeOfBlock - sizeof(EFI_IMAGE_BASE_RELOCATION)) / 2;
            UINT16 *reloc = (UINT16 *) (p_reloc + 1);

            for (int i = 0; i < size; i++) {
                int type = reloc[i] >> 12;
                int offset = reloc[i] & 0xFFF;
                UINT64 *change_addr = (UINT64 *) (RVA_to_VA(ImageBase, p_reloc->VirtualAddress) + offset);

                switch (type) {
                    case EFI_IMAGE_REL_BASED_ABSOLUTE:
                        break;

        			case EFI_IMAGE_REL_BASED_HIGH:
        				UINT16 *fixup16 = (UINT16 *) change_addr;
        				*fixup16 = (UINT16) (*fixup16 + ((UINT16) ((UINT32) delta_VA_reloc >> 16)));
        				break;

        			case EFI_IMAGE_REL_BASED_LOW:
        				UINT16 *fixup16 = (UINT16 *) change_addr;
        				*fixup16  = (UINT16) (*fixup16 + (UINT16) delta_VA_reloc);
        				break;

                    case EFI_IMAGE_REL_BASED_HIGHLOW:
                        UINT32 *fixup32 = (UINT32 *) change_addr;
                        *fixup32 = *fixup32 + (UINT32) delta_VA_reloc;
                        break;

                    case EFI_IMAGE_REL_BASED_DIR64:
                        UINT64 *fixup64 = (UINT64 *) change_addr;
                        *fixup64 = *fixup64 + (UINT64) delta_VA_reloc;
                        break;

                    default:
                        break;
                }
            }

            p_reloc = ((UINT8 *) p_reloc + p_reloc->SizeOfBlock);
        }
    }

    EFI_LOADED_IMAGE_PROTOCOL new_loaded_image_protocol = {
        EFI_LOADED_IMAGE_PROTOCOL_REVISION, //     UINT32            Revision;       ///< Defines the revision of the EFI_LOADED_IMAGE_PROTOCOL structure.
        gImageHandle,                       //     EFI_HANDLE        ParentHandle;   ///< Parent image's image handle. NULL if the image is loaded directly from
        gST,                                //     EFI_SYSTEM_TABLE  *SystemTable;   ///< the image's EFI system table pointer.
        NULL,                               //     source location DeviceHandle, ignored
        NULL,                               //     source location DEVICE_PATH_PROTOCOL, inored
        NULL,                               //     reserved
        0,                                  //     UINT32            LoadOptionsSize;///< The size in bytes of LoadOptions.
        NULL,                               //     VOID              *LoadOptions;   ///< A pointer to the image's binary load options.
        ImageBase,                          //     VOID              *ImageBase;     ///< The base address at which the image was loaded.
        size_of_image,                      //     UINT64            ImageSize;      ///< The size in bytes of the loaded image.
        EfiLoaderCode,                      //     EFI_MEMORY_TYPE   ImageCodeType;  ///< The memory type that the code sections were loaded as.
        EfiLoaderCode,                      //     EFI_MEMORY_TYPE   ImageDataType;  ///< The memory type that the data sections were loaded as.};
    EFI_GUID loaded_image_protocol_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;

    status = gBS->InstallMultipleProtocolInterfaces(
        new_image_handle,
        &loaded_image_protocol_guid);

    return status;
}

EFI_STATUS EFIAPI start_image_from_mem(EFI_HANDLE new_image_handle) {
    EFI_STATUS status;

    EFI_LOADED_IMAGE_PROTOCOL loaded_image_protocol;
    EFI_GUID loaded_image_protocol_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    status = gBS->OpenProtocol(
        new_image_handle,
        &loaded_image_protocol_guid,
        &loaded_image_protocol
        gImageHandle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(status)) {
        return status;
    }

    EFI_IMAGE_DOS_HEADER *p_DOS_HDR = (EFI_IMAGE_DOS_HEADER *) application_ptr;
    EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION p_NT_HDR = (EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION *) (((char*) DosHdr) + DosHdr->e_lfanew);
    UINT64 entry_point_RVA = p_NT_HDR->OptionalHeader.AddressOfEntryPoint;
    UINT64 entry_point_VA = RVA_to_VA(loaded_image_protocol->ImageBase, entry_point_RVA);

    EFI_STATUS (*entry)(IN EFI_HANDLE, IN EFI_SYSTEM_TABLE*);
    entry = entry_point_VA;
    status = entry(new_image_handle, loaded_image_protocol->SystemTable);
    return status;
}


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

    gRT->SetVariable(
        APPLICATION_VAR,
        &application_var_guid,
        EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
        application_var_size,
        application_ptr);

    EFI_HANDLE new_image_handle;
    load_image_from_mem(application_ptr, &new_image_handle);
    status = start_image_from_mem(new_image_handle);

    return status;
}
