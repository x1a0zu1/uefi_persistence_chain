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

-> diagram
initial infection:
                                      flash    __________________
    intel NIC firmware update utility ----->  | option rom       |
                                              | ---------------- |
                                              | chainloader stub |
                                              |__________________|

                         SetVariable()    _______________________
    EFI runtime services ------------->  | NVRAM                 |
                                         | --------------------- |
                                         | compressed dxe driver |
                                         |_______________________|

next reboot:
                   load and execute    ___________________
    DXE dispatcher ---------------->  | main memory       |
                        ^             | ----------------- | GetVariable()    _______________________
                        |             | chainloader stub  | --------------> | main memory           |
                    option rom        |___________________|        ^        | --------------------- | decompress, manually map then jump to entry
                    <chainloader stub>                             |        | chainloader stub      | -------
                                                                   |        | ...                   |        |
                                                                   |        | compressed dxe driver | <------
                                                                   |        |_______________________| ------------------> dxe driver executes
                                                                   |
                                                                 NVRAM
                                                                 <compressed dxe driver>

ransomware:
----------------
RAMIEL currently loads a simple < 25kb uefi ransomware, which is kind of pointless since ransomware doesnt need persistence but oh well.
the ransomware itself is nothing remarkable, it is meant to be small so as to fit in a single nvram variable and-
uses diskio to avoid the bloat of specific filesystem drivers, making it entirely OS agnostic. it will eventually use AES-NI-
for faster encryption but im too lazy to wrangle with openssl currently.

the ransomware will display a fake firmware update message warning victims not to reboot to maximize damage done.
as there are no AVs to worry about, the ransomware might actually be quite effective.

todo:
----------------
this project is a WORK IN PROGRESS and has not been tested on bare metal as of yet.
i intend to implement an OVA-based infection method as well as a simple packer to get binary sizes as small as possible.
i may also implement the ability to break a large binary into chunks to bypass the max NVRAM variable size limit.
