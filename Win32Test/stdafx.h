#ifndef _stdafx_
#define _stdafx_
#include<windows.h>
#include<stdio.h>
#include<iostream>
#include <tchar.h>
#include<queue>
#include<locale.h>
#include <commctrl.h>
#include<tlhelp32.h>
#include <string>
#include<map>
#include<stringapiset.h>
#include <psapi.h>
#pragma comment(lib,"comctl32.lib")
#include"resource1.h"
using namespace std;

void __cdecl OutputDebugStringF(const char* format, ...);
void __cdecl OutputDebugStringWF(const wchar_t* format, ...);
#ifdef _DEBUG 
#define DbgPrintf OutputDebugStringF
#else
#define DbgPrintf
#endif

#ifdef _DEBUG 
#define DbgwPrintf OutputDebugStringWF
#else
#define DbgwPrintf
#endif

void FileToFileBuffer(char* PATH, LPVOID* pFileBuffer);
void FileBufferToImageBuffer(LPVOID pFileBuffer, LPVOID* pImageBuffer);
void ImageBufferToNewBuffer(LPVOID pImageBuffer, LPVOID* pNewBuffer);
void NewBufferToFile(LPVOID pNewBuffer, char* PATH);
void AddCode(LPVOID pImageBuffer, char* text);
void AddSectionToImageBuffer(LPVOID* pImageBuffer);
void ShiftSectionHeader(LPVOID pFileBuffer);
int Alignment(int num, int AlignmentNum);
void ExLastSectionToImageBuffer(LPVOID* pImageBuffer);
void PrintAll(LPVOID pImageBuffer);
void SumSection(LPVOID pImageBuffer);
DWORD RVATOFOA(LPVOID pFileBuffer, DWORD RVA);
DWORD FOATORVA(LPVOID pFileBuffer, DWORD FOA);
void ExportTablePrint(LPVOID pFileBuffer);
DWORD GetFunctionAddrByName(LPVOID pFileBuffer, char* text);
DWORD GetFunctionAddrByOrdinals(LPVOID pFileBuffer, DWORD NUM);
void PrintBaseRelocTableAll(LPVOID pFileBuffer);
void MoveTableToNewSection(LPVOID pFileBuffer, DWORD FOA);
DWORD AddSectionToFileBuffer(LPVOID* pFileBuffer, char* text, DWORD SizeOfSection);
void ChangeImageBase(LPVOID pFileBuffer, DWORD ImageBase);
void PrintImportTable(LPVOID pFileBuffer);
void PrintBoundImportTable(LPVOID pFileBuffer);
void InjectImportTable(LPVOID pFileBuffer, char* SectionName, char* FunctionName, char* dllName);
void ResourceTablePrint(LPVOID pFileBuffer);
void ResourceTablePrintDFS(LPVOID pFileBuffer, DWORD OFFSET, DWORD lay, DWORD Base);

void ShowAllProc(HWND hWndProc);
void ShowAllModules(HWND hWndModule, DWORD PID);
void OpenFileButton();
BOOL CALLBACK Dlgproc_Main(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK Dlgproc_PE(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK Dlgproc_About(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK Dlgproc_FileHeader(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK Dlgproc_OptionalHeader(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK Dlgproc_Sections(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK Dlgproc_DataDirectory(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK Dlgproc_ImportEx(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK Dlgproc_ExportEx(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK Dlgproc_ResourceEx(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK Dlgproc_BoundEx(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK Dlgproc_RelocEx(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK Dlgproc_IATEx(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
void InitListControlMain(HWND hWnd);
void ShowDosHeader(HWND hWndPE);
void InitListControlSection(HWND hWnd);
void ShowAllSection(HWND hWndSection);
void ShowFileHeader(HWND hWndFH);
void ShowExportTable(HWND hWndET);
void ShowOptionalHeader(HWND hWndOH);
void ShowDataDirectory(HWND hWndDD);
void InitListControlExportTable(HWND hWnd);
void InitListControlImportTable(HWND hWnd);
void ShowAllExport(HWND hWndET);
void ShowDllImport(HWND HWNDIT);
void ShowAPIImport(HWND HWNDIT, DWORD OFT_RVA, DWORD FT_RVA);
void ShowAllResource(HWND hWndRL);
void ShowAllResourceDFS(HWND hWndRL, DWORD OFFSET,  DWORD Base, HTREEITEM fatherNode);
void ShowAllRelocBlock(HWND hWndRL);
void InitListControlRelocTable(HWND hWnd);
void ShowAllRelocBlockINFO(HWND hWndRLIF, DWORD Index);
void InitListControlBoundImport(HWND hWnd);
void ShowAllBoundImport(HWND hWndBI);
void ShowSelectItemResource(HWND HWNDSR, DWORD OFFSET);
void ShowSelectDirResource(HWND HWNDSD, DWORD OFFSET);

#endif
