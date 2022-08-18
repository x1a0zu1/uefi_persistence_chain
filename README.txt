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
| -> persistence via flashing PCI option rom DXE drivers, |
     which manually map arbitrary code stored in nvram    |
| -> POC uefi ransomware                                  |
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

abstract:
----------------
during the DXE phase of uefi, drivers are discovered, loaded and executed automatically from a variety-
of locations, including the option ROMs of PCI devices.
as PCI option rom is used for configuration, it cannot be signed for secure boot by nature.

signatures are often checked during firmware updates, but are not verified for most intel NICs.
thus, it is possible to flash the NICs option ROM with unsigned code.

PCI option rom is often the target for persistence but it is limited by its small size-
as well as its ability to be easily dumped from OS by firmware update utilities.
NVRAM variables without the RUNTIME_ACCESS flag set cannot be dumped easily from OS-
but code stored in NVRAM is never executed automatically.

RAMIEL presents a novel persistence "chain" that attempts to remedy these limitations.
via a small stub stored in PCI option rom that decompresses manually maps a compressed DXE driver stored in nvram,
RAMIEl can prevent the compressed driver from being easily dumped from OS,-
utilize around ~300kb (on average) of extra storage, and execute code during DXE.

** since the driver is manually mapped, RAMIEL will function with secure boot enabled.
** since RAMIEL persists entirely off disk, it will survive OS re-installation, complete hard drive wipes-
   and even persist in diskless netboot systems.

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
                                         | compressed DXE driver |
                                         |_______________________|

next reboot:
                   load and execute    ___________________
    DXE dispatcher ---------------->  | main memory       |
                        ^             | ----------------- | GetVariable()    __________________
                        |             | chainloader stub  | --------------> | main memory      | SetVariable()
                    option rom        |___________________|        ^        | ---------------- | -------------> remove RUNTIME_ACCESS flag
                    <chainloader stub>                             |        | chainloader stub | -------
                                                                   |        | ...              |        | decompress, manually map then jump to entry
                                                                   |        | DXE driver       | <------
                                                                   |        |__________________| ------------------> DXE driver executes
                                                                   |
                                                                 NVRAM
                                                                 <compressed DXE driver>

ransomware:
----------------
RAMIEL currently loads a simple < 25kb uefi ransomware, which is kind of pointless since ransomware doesnt need persistence but oh well.
the ransomware itself is nothing remarkable, it is meant to be small in order to fit in a single nvram variable and-
uses diskio to avoid the overhead of filesystem specific drivers, making it entirely OS agnostic. it will eventually use AES-NI-
for faster encryption but currently i am too lazy to wrangle with openssl currently.

the ransomware displays a fake firmware update message warning victims not to reboot to maximize damage done.
as there are no AVs to worry about, the ransomware may be quite effective.

todo:
----------------
this project is a WORK IN PROGRESS and has not been tested on bare metal as of yet.
i intend to implement an OVA based infection method as well as a simple packer to get binary sizes as small as possible.
i may also implement the ability to break a large binary into chunks to bypass the max NVRAM variable size limit.
