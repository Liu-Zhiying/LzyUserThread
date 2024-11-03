#ifndef CONTEXT_H
#define CONTEXT_H

#include <Windows.h>

#define CAN_RESTORE 0x1

#if defined(_M_X64)

typedef UINT64 QWORD;

typedef struct UserThreadContext
{
    QWORD   Rax;                    //+0x0                  
    QWORD   Rbx;                    //+0x8
	QWORD   Rcx;                    //+0x10
	QWORD   Rdx;                    //+0x18
	QWORD   Rsi;                    //+0x20
	QWORD   Rdi;                    //+0x28
	QWORD   R8;                     //+0x30
	QWORD   R9;                     //+0x38
	QWORD   R10;                    //+0x40
	QWORD   R11;                    //+0x48
	QWORD   R12;                    //+0x50
	QWORD   R13;                    //+0x58
	QWORD   R14;                    //+0x60
	QWORD   R15;                    //+0x68
    QWORD   Rsp;                    //+0x70
    QWORD   Rip;                    //+0x78
    QWORD   RFlags;                 //+0x80
    QWORD   Unused;                 //+0x88

    DWORD   Xmm0[4];                //+0x90
    DWORD   Xmm1[4];                //+0xa0
    DWORD   Xmm2[4];                //+0xb0
    DWORD   Xmm3[4];                //+0xc0
    DWORD   Xmm4[4];                //+0xd0
    DWORD   Xmm5[4];                //+0xe0
    DWORD   Xmm6[4];                //+0xf0
    DWORD   Xmm7[4];                //+0x100
    DWORD   Xmm8[4];                //+0x110
    DWORD   Xmm9[4];                //+0x120
	DWORD   Xmm10[4];               //+0x130
	DWORD   Xmm11[4];               //+0x140
	DWORD   Xmm12[4];               //+0x150
	DWORD   Xmm13[4];               //+0x160
	DWORD   Xmm14[4];               //+0x170
	DWORD   Xmm15[4];               //+0x180

    PVOID   pStack;                 //+0x190
    SIZE_T  StackSize;              //+0x198

    //¶Î¼Ä´æÆ÷Î´Ê¹ÓÃ
    DWORD   SegSs;                  //+0x1a0
    DWORD   SegGs;                  //+0x1a4
    DWORD   SegFs;                  //+0x1a8
    DWORD   SegEs;                  //+0x1ac
    DWORD   SegDs;                  //+0x1b0
    DWORD   SegCs;                  //+0x1b4

    DWORD UserThreadFlags;          //+0x1b8
} UserThreadContext, * PUserThreadContext;

#elif defined(_M_IX86)

typedef struct UserThreadContext
{
    //¶Î¼Ä´æÆ÷Î´Ê¹ÓÃ
    DWORD   SegSs;                  //+0x0
    DWORD   SegGs;                  //+0x4
    DWORD   SegFs;                  //+0x8
    DWORD   SegEs;                  //+0xc
    DWORD   SegDs;                  //+0x10
    DWORD   SegCs;                  //+0x14

    DWORD   Edi;                    //+0x18
    DWORD   Esi;                    //+0x1c
    DWORD   Ebx;                    //+0x20
    DWORD   Edx;                    //+0x24
    DWORD   Ecx;                    //+0x28
    DWORD   Eax;                    //+0x2c
    DWORD   Ebp;                    //+0x30
    DWORD   Eip;                    //+0x34
    DWORD   Esp;                    //+0x38

    DWORD   EFlags;                 //+0x3c

    DWORD   Xmm0[4];                //+0x40
    DWORD   Xmm1[4];                //+0x50
    DWORD   Xmm2[4];                //+0x60
    DWORD   Xmm3[4];                //+0x70
    DWORD   Xmm4[4];                //+0x80
    DWORD   Xmm5[4];                //+0x90
    DWORD   Xmm6[4];                //+0xa0
    DWORD   Xmm7[4];                //+0xb0

    DWORD UserThreadFlags;          //+0xc0

    PVOID   pStack;
    SIZE_T  StackSize;
} UserThreadContext, * PUserThreadContext;

#endif

extern "C" DWORD __fastcall SaveContext(PUserThreadContext pContext);
extern "C" DWORD __fastcall RestoreContext(PUserThreadContext pContext);

#endif