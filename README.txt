                    #
                  %%&
              ,%%%%%%    #,
            %%%%%%%%%       (
         *%%%%%%%%%%%
      #%%%%%%%%%%%@. &%@.        /                           _     __
    %%%%%%& %%%%%%%% %%%%%%%%%@     (        _______ ___ _  (_)__ / /
 @*%%%%%%%%%%%%%%%%% %&%%%%%%%%%%%%%%@ /    / __/ _ `/  ' \/ / -_) /
   &@@@&%%%%%%%%%%%% %%%%%%%%%%%%%%%% /    /_/  \_,_/_/_/_/_/\__/_/
      @@@@@@@@%%%%%% %%%%%%%%%%%%%         ^^^^^^^^^^^^^^ 0xwillow
        %@@@@&@@@@@@ %%%%%%%%%%&/
           #@@@@@@@@ %%%%%%%%*
              #@@@&@ %%%%%(
                 @@@.%%%
                   #%#

<RAMIEL POC>
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
| -> persistence via flashing PCI option rom dxe drivers, |
     which manually map arbitrary code stored in nvram    |
| -> POC uefi ransomware                                  |
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

abstract:
----------------
during the DXE phase of uefi, drivers are discovered, loaded and executed automatically from a variety-
of locations, including the option ROMs of PCI devices.
as PCI option rom is used for configuration, it cannot be signed for secure boot by nature.

signatures are often checked at the firmware update level, but are not for most intel NICs.
thus, it is possible to flash the NICs option ROM with unsigned code.

PCI option rom is often the target for persistence but it is limited by its small size-
as well as its ability to be easily dumped from OS by firmware update utilities.
NVRAM variables without the RUNTIME_ACCESS flag set cannot be dumped easily from OS-
but code stored in NVRAM is never executed automatically.

RAMIEL presents a novel persistence method that attempts to fix these limitations-
via a small stub stored in PCI option rom that chainloads and decompresses a compressed dxe driver stored in nvram,
RAMIEl cant prevent the compressed driver from being easily dumped from OS.
and as the loaded driver is manually mapped RAMIEL will work with secure boot enabled.
RAMIEL will survive complete hard drive wipes as it persists entirely off disk-

work in progress...
