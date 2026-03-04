# GaggiaControllerX
Controller to automate Gaggia expresso machine

This Project relies in the SDK NRF52 environment.
nRF5 SDK version: nRF5_SDK_17.1.0_ddde560

This project was based on the follwing youtube video: https://www.youtube.com/watch?v=8drz4rDqswo&t=20s&ab_channel=nrf5dev


Trying to compile project in a SEGGER Embedded Studio -> Release 8.26c Build 2026021300.61129 or higher?
---------------------------------------------------------------------------------------------------------------------------------------------------------
Then you find the following problems with 
- SEGGER_RTT.c related and msg error: "__vfprintf.h: No such file or directory"

Inside flash_placement.xml, replace following lines:
 -     <ProgramSection alignment="4" load="Yes" name=".text" size="0x4" />  to     <ProgramSection alignment="4" load="Yes" name=".text" />
 -     <ProgramSection alignment="4" load="Yes" name=".rodata" size="0x4" />  to     <ProgramSection alignment="4" load="Yes" name=".rodata" />
Forum fix soruce:
- https://devzone.nordicsemi.com/f/nordic-q-a/89236/build-error
- https://devzone.nordicsemi.com/f/nordic-q-a/94689/stuck-at-building-blinky-example-text-is-larger-than-specified-size 

Then: remove the SEGGER_RTT_Syscalls_SES.c from project
Forum fix soruce: 
- https://devzone.nordicsemi.com/f/nordic-q-a/85919/__vfprintf-h-no-such-file-or-directory 

Then: Under 'Solution->Options->Common->Private Configurations->Library' changed 'Library I/O' to 'RTT'
Forum fix soruce:  
- https://devzone.nordicsemi.com/f/nordic-q-a/105785/library-heap-not-functional-when-migrating-from-ses-5-70a-to-ses-7-32

Trying to compile project from a new location?
Long-term solution, use macro to poin to the SDK, e.g.
Got to Solution/Project → Edit Options (Common) → Build → Project Macros
- SDK_ROOT = C:/WS/NRF/nRF5_SDK_17.1.0_ddde560
Make sure your project.emProject file also reflect this change
- $(SDK_ROOT)/components/
- etc

Forum fix soruce:  
- https://devzone.nordicsemi.com/f/nordic-q-a/62122/best-practices-for-starting-a-new-project-in-ses-based-on-example-application/253302#253302

