```
*******************************************************************************
*                                                                             *
*                        Bugcheck Analysis                                    *
*                                                                             *
*******************************************************************************

KERNEL_SECURITY_CHECK_FAILURE (139)
A kernel component has corrupted a critical data structure.  The corruption
could potentially allow a malicious user to gain control of this machine.
Arguments:
Arg1: 0000000000000003, A LIST_ENTRY has been corrupted (i.e. double remove).
Arg2: ffffa90fa4fe2370, Address of the trap frame for the exception that caused the BugCheck
Arg3: ffffa90fa4fe22c8, Address of the exception record for the exception that caused the BugCheck
Arg4: 0000000000000000, Reserved

Debugging Details:
------------------


KEY_VALUES_STRING: 1

    Key  : Analysis.CPU.mSec
    Value: 10093

    Key  : Analysis.DebugAnalysisManager
    Value: Create

    Key  : Analysis.Elapsed.mSec
    Value: 49367

    Key  : Analysis.Init.CPU.mSec
    Value: 69921

    Key  : Analysis.Init.Elapsed.mSec
    Value: 13952566

    Key  : Analysis.Memory.CommitPeak.Mb
    Value: 188

    Key  : FailFast.Name
    Value: CORRUPT_LIST_ENTRY

    Key  : FailFast.Type
    Value: 3

    Key  : WER.OS.Branch
    Value: vb_release

    Key  : WER.OS.Timestamp
    Value: 2019-12-06T14:06:00Z

    Key  : WER.OS.Version
    Value: 10.0.19041.1


BUGCHECK_CODE:  139

BUGCHECK_P1: 3

BUGCHECK_P2: ffffa90fa4fe2370

BUGCHECK_P3: ffffa90fa4fe22c8

BUGCHECK_P4: 0

TRAP_FRAME:  ffffa90fa4fe2370 -- (.trap 0xffffa90fa4fe2370)
NOTE: The trap frame does not contain all registers.
Some register values may be zeroed or incorrect.
rax=ffffab0fa9fecd40 rbx=0000000000000000 rcx=0000000000000003
rdx=ffffab0fa217ce30 rsi=0000000000000000 rdi=0000000000000000
rip=fffff807501d2669 rsp=ffffa90fa4fe2508 rbp=0000000000000015
 r8=ffffab0fa3b49bb0  r9=0000000000000015 r10=ffffab0f9f2d7118
r11=ffffab0f9f2d7010 r12=0000000000000000 r13=0000000000000000
r14=0000000000000000 r15=0000000000000000
iopl=0         nv up ei ng nz na po cy
FLTMGR!FltpInsertCallback+0x7b55:
fffff807`501d2669 cd29            int     29h
Resetting default scope

EXCEPTION_RECORD:  ffffa90fa4fe22c8 -- (.exr 0xffffa90fa4fe22c8)
ExceptionAddress: fffff807501d2669 (FLTMGR!FltpInsertCallback+0x0000000000007b55)
   ExceptionCode: c0000409 (Security check failure or stack buffer overrun)
  ExceptionFlags: 00000001
NumberParameters: 1
   Parameter[0]: 0000000000000003
Subcode: 0x3 FAST_FAIL_CORRUPT_LIST_ENTRY 

PROCESS_NAME:  System

ERROR_CODE: (NTSTATUS) 0xc0000409 - The system detected an overrun of a stack-based buffer in this application. This overrun could potentially allow a malicious user to gain control of this application.

EXCEPTION_CODE_STR:  c0000409

EXCEPTION_PARAMETER1:  0000000000000003

EXCEPTION_STR:  0xc0000409

STACK_TEXT:  
ffffa90f`a4fe1898 fffff807`51512f02     : ffffa90f`a4fe1a00 fffff807`5137d8e0 00000000`00000100 00000000`00000000 : nt!DbgBreakPointWithStatus
ffffa90f`a4fe18a0 fffff807`515124e6     : 00000000`00000003 ffffa90f`a4fe1a00 fffff807`5140dec0 00000000`00000139 : nt!KiBugCheckDebugBreak+0x12
ffffa90f`a4fe1900 fffff807`513f90a7     : 00000000`00000000 00000000`00000000 ffffab0f`a9fec7a8 ffffa90f`a4fe2240 : nt!KeBugCheck2+0x946
ffffa90f`a4fe2010 fffff807`5140af69     : 00000000`00000139 00000000`00000003 ffffa90f`a4fe2370 ffffa90f`a4fe22c8 : nt!KeBugCheckEx+0x107
ffffa90f`a4fe2050 fffff807`5140b390     : 00000000`00000000 00000000`00000000 ffffa90f`a4fe21a0 ffffa90f`a4fe21a0 : nt!KiBugCheckDispatch+0x69
ffffa90f`a4fe2190 fffff807`51409723     : 00000000`00000000 ffffa90f`a4fe2480 00000000`00000270 ffffab0f`9f8cf9f0 : nt!KiFastFailDispatch+0xd0
ffffa90f`a4fe2370 fffff807`501d2669     : fffff807`501caa95 ffffab0f`a3e98d10 ffffab0f`a3e98d10 00000000`00000000 : nt!KiRaiseSecurityCheckFailure+0x323
ffffa90f`a4fe2508 fffff807`501caa95     : ffffab0f`a3e98d10 ffffab0f`a3e98d10 00000000`00000000 ffffab0f`a9fecf80 : FLTMGR!FltpInsertCallback+0x7b55
ffffa90f`a4fe2510 fffff807`501ff5fa     : ffffab0f`a9fec660 ffffa90f`a4fe2670 ffffab0f`9f2d7010 00000000`00000000 : FLTMGR!FltpSetCallbacksForInstance+0x1f1
ffffa90f`a4fe2570 fffff807`501ff056     : 00000000`00000000 ffffa90f`a4fe26a1 00000000`00000001 00000000`00000010 : FLTMGR!FltpInitInstance+0x432
ffffa90f`a4fe2610 fffff807`501fee14     : 00000000`00000000 ffffab0f`9f2d7010 00000000`00000050 00000000`0000001a : FLTMGR!FltpCreateInstanceFromName+0x1de
ffffa90f`a4fe26f0 fffff807`5020a6cd     : ffffab0f`00000046 ffffab0f`9f2d7020 ffffab0f`9f2d7020 ffffffff`8000361c : FLTMGR!FltpEnumerateRegistryInstances+0xe0
ffffa90f`a4fe2780 fffff807`5020a5eb     : 00000000`00000000 ffffa90f`a4fe2a60 ffffab0f`aa6ff000 ffffe287`41513520 : FLTMGR!FltpDoVolumeNotificationForNewFilter+0xa5
ffffa90f`a4fe27c0 fffff808`22f635e9     : 00000000`00000000 ffffab0f`aa6ff000 ffffab0f`aacbfe10 ffffe287`41513520 : FLTMGR!FltStartFiltering+0x2b
ffffa90f`a4fe2800 fffff808`22f67894     : ffffab0f`abceb4d0 ffffe287`47acee10 ffffa90f`a4fe2a60 00000000`00000005 : PROCMON24+0x35e9
ffffa90f`a4fe28a0 fffff808`22f74020     : ffffab0f`aa6ff000 ffffab0f`abceb4d0 00000000`00000002 ffffffff`800047d4 : PROCMON24+0x7894
ffffa90f`a4fe28d0 fffff807`51761a2c     : ffffab0f`aa6ff000 00000000`00000000 ffffab0f`aacbfe10 00000000`00000000 : PROCMON24+0x14020
ffffa90f`a4fe2900 fffff807`5172d1bd     : 00000000`00000014 00000000`00000000 00000000`00000000 00000000`00001000 : nt!PnpCallDriverEntry+0x4c
ffffa90f`a4fe2960 fffff807`517724c7     : 00000000`00000000 00000000`00000000 fffff807`51d25440 00000000`00000000 : nt!IopLoadDriver+0x4e5
ffffa90f`a4fe2b30 fffff807`51252b65     : ffffab0f`00000000 ffffffff`800047d4 ffffab0f`a16f0040 ffffab0f`00000000 : nt!IopLoadUnloadDriver+0x57
ffffa90f`a4fe2b70 fffff807`51271d25     : ffffab0f`a16f0040 00000000`00000080 ffffab0f`9ecaa040 00000000`00000000 : nt!ExpWorkerThread+0x105
ffffa90f`a4fe2c10 fffff807`51400628     : ffffcd81`fa2ec180 ffffab0f`a16f0040 fffff807`51271cd0 00000000`00000246 : nt!PspSystemThreadStartup+0x55
ffffa90f`a4fe2c60 00000000`00000000     : ffffa90f`a4fe3000 ffffa90f`a4fdd000 00000000`00000000 00000000`00000000 : nt!KiStartSystemThread+0x28


SYMBOL_NAME:  PROCMON24+35e9

MODULE_NAME: PROCMON24

IMAGE_NAME:  PROCMON24.SYS

IMAGE_VERSION:  3.91.0.0

STACK_COMMAND:  .cxr; .ecxr ; kb

BUCKET_ID_FUNC_OFFSET:  35e9

FAILURE_BUCKET_ID:  0x139_3_CORRUPT_LIST_ENTRY_PROCMON24!unknown_function

OS_VERSION:  10.0.19041.1

BUILDLAB_STR:  vb_release

OSPLATFORM_TYPE:  x64

OSNAME:  Windows 10

FAILURE_ID_HASH:  {5dbfd5b7-3954-0a5c-1c1a-36b5712f0de8}

Followup:     MachineOwner
---------
```