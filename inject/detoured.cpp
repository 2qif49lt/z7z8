//////////////////////////////////////////////////////////////////////
//
//  Module:     detoured.dll
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Microsoft Research Detours Package, Version 2.1 (Build_207)
//

#include <windows.h>
#include "detours.h"
#include "detoured.h"

static HMODULE s_hDll;

#include "Log_/logx.h"
using namespace xhb;
PIMAGE_DOS_HEADER  pDosHeader;
PIMAGE_NT_HEADERS  pNTHeaders;
PIMAGE_OPTIONAL_HEADER   pOptHeader;
PIMAGE_IMPORT_DESCRIPTOR  pImportDescriptor;
PIMAGE_THUNK_DATA         pThunkData;
PIMAGE_IMPORT_BY_NAME     pImportByName;
HMODULE hMod;
logx<criticalsectionx_> g_log;

// 定义MessageBoxA函数原型
typedef int (WINAPI *PFNMESSAGEBOX)(HWND, LPCSTR, LPCSTR, UINT uType);
int WINAPI MessageBoxProxy(IN HWND hWnd, IN LPCSTR lpText, IN LPCSTR lpCaption, IN UINT uType);

typedef BOOL (WINAPI* PExtTextOutWin)(HDC , int , int , UINT , CONST RECT *, LPCTSTR , UINT , CONST INT *);
BOOL WINAPI ExtTextOutProxy(HDC hdc, int X, int Y, UINT fuOptions, CONST RECT *lprc, LPCTSTR lpString, UINT cbCount, CONST INT *lpDx);

int * addr = (int *)MessageBoxA;    //保存函数的入口地址
int * myaddr = (int *)MessageBoxProxy;

int * extaddr = (int*)ExtTextOutA;
int* myextaddr = (int*)ExtTextOutProxy;

typedef BOOL(WINAPI* PsysTextOutA)(  HDC ,  int ,  int ,  LPCSTR lpString,  int c);
BOOL  WINAPI MyTextOutA( __in HDC hdc, __in int x, __in int y,  LPCSTR lpString, __in int c);
int* ptextout = (int*)TextOutA;
int* pmytextout = (int*)MyTextOutA;

typedef int (WINAPI *psysDrawTextA)( HDC ,LPCSTR ,int ,LPRECT ,UINT );
int WINAPI MyDrawTextA( HDC hdc,LPCSTR lpchText,int cchText,LPRECT lprc,UINT format);
int* pdrawtext = (int*)DrawTextA;
int* pmydrawtext = (int*)MyDrawTextA;

typedef int (WINAPI *psysDrawTextW)( HDC ,LPCWSTR ,int ,LPRECT ,UINT );
int WINAPI MyDrawTextW( HDC hdc,LPCWSTR lpchText,int cchText,LPRECT lprc,UINT format);
int* pdrawtextw = (int*)DrawTextW;
int* pmydrawtextw = (int*)MyDrawTextW;

void ThreadProc(void *param);//线程函数

HMODULE WINAPI Detoured()
{
    return s_hDll;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    (void)reserved;

    if (dwReason == DLL_PROCESS_ATTACH) {
		g_log.InitLog("dlllog","base.log",LOG_LV_NORMAL);
		g_log.log("dll inject",LOG_LV_INFO);
        s_hDll = hinst;
        DisableThreadLibraryCalls(hinst);
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&(PVOID&)pdrawtext, pmydrawtext);
		if(DetourTransactionCommit() == NO_ERROR)
			g_log.log(LOG_LV_INFO,"%s","DrawTextA hooked");

		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&(PVOID&)extaddr, myextaddr);
		if(DetourTransactionCommit() == NO_ERROR)
			g_log.log(LOG_LV_INFO,"%s","ExtTextOutA hooked");
    }
    return TRUE;
}

//
///////////////////////////////////////////////////////////////// End of File.

void ThreadProc(void *param)
{
	
	
	g_log.InitLog("dlllog","base.log",LOG_LV_NORMAL);
	g_log.log("dll inject",LOG_LV_INFO);

	//------------hook api----------------
	hMod = GetModuleHandle(NULL);

	pDosHeader = (PIMAGE_DOS_HEADER)hMod;
	pNTHeaders = (PIMAGE_NT_HEADERS)((BYTE *)hMod + pDosHeader->e_lfanew);
	pOptHeader = (PIMAGE_OPTIONAL_HEADER)&(pNTHeaders->OptionalHeader);

	pImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((BYTE *)hMod + pOptHeader->DataDirectory[1].VirtualAddress);

	while(pImportDescriptor->FirstThunk)
	{
		char * dllname = (char *)((BYTE *)hMod + pImportDescriptor->Name);

		pThunkData = (PIMAGE_THUNK_DATA)((BYTE *)hMod + pImportDescriptor->OriginalFirstThunk);

		int no = 1;
		while(pThunkData->u1.Function)
		{
			char * funname = (char *)((BYTE *)hMod + (DWORD)pThunkData->u1.AddressOfData + 2);
			PDWORD lpAddr = (DWORD *)((BYTE *)hMod + (DWORD)pImportDescriptor->FirstThunk) +(no-1);

			//修改内存的部分
			if((*lpAddr) == (int)pdrawtextw)
			{
				g_log.log(LOG_LV_INFO,"%s","hooked");
				//修改内存页的属性
				DWORD dwOLD;
				MEMORY_BASIC_INFORMATION  mbi;
				VirtualQuery(lpAddr,&mbi,sizeof(mbi));
				VirtualProtect(lpAddr,sizeof(DWORD),PAGE_READWRITE,&dwOLD);

				WriteProcessMemory(GetCurrentProcess(), 
					lpAddr, &pmydrawtextw, sizeof(DWORD), NULL);
				//恢复内存页的属性
				VirtualProtect(lpAddr,sizeof(DWORD),dwOLD,0);
			}
			//---------
			no++;
			pThunkData++;
		}

		pImportDescriptor++;
	}
	//-------------------HOOK END-----------------
}

//new messagebox function
int WINAPI MessageBoxProxy(IN HWND hWnd, IN LPCSTR lpText, IN LPCSTR lpCaption, IN UINT uType)
{
	g_log.log(LOG_LV_INFO,"%s",lpText);
	return     ((PFNMESSAGEBOX)addr)(hWnd, lpText, lpCaption, uType);
	//这个地方可以写出对这个API函数的处理代码
}
BOOL WINAPI ExtTextOutProxy(HDC hdc, int X, int Y, UINT fuOptions, CONST RECT *lprc, LPCTSTR lpString, UINT cbCount, CONST INT *lpDx)
{
	g_log.log(LOG_LV_WARN,"%d-[%s]",cbCount,lpString);
	return ((PExtTextOutWin)extaddr)(hdc,X,Y,fuOptions,lprc,lpString,cbCount,lpDx);
}


BOOL WINAPI MyTextOutA(__in HDC hdc, __in int x, __in int y, LPCSTR lpString, __in int c)
{
//	g_log.log(LOG_LV_FAILD,"%s",lpString);
	return ((PsysTextOutA)ptextout)(hdc,x,y,lpString,c);
}
int WINAPI MyDrawTextA(HDC hdc,LPCSTR lpchText,int cchText,LPRECT lprc,UINT format)
{
	g_log.log(LOG_LV_INFO,"%d-[%s]",cchText,lpchText);
	return ((psysDrawTextA)pdrawtext)(hdc,lpchText,cchText,lprc,format);
}
int WINAPI MyDrawTextW(HDC hdc,LPCWSTR lpchText,int cchText,LPRECT lprc,UINT format)
{
	//g_log.log(LOG_LV_INFO,"%s",lpchText);
	return ((psysDrawTextW)pdrawtextw)(hdc,lpchText,cchText,lprc,format);
} 