#define _CRT_SECURE_NO_WARNINGS

#define MessageBoxAddress 0x75FCC730
#include "stdafx.h"

//HWND MAINHWND;
HWND PROCHWND;
HWND MODULEHWND;
HWND SECTIONHWND;
TCHAR szFile[MAX_PATH];
LPVOID FileBuffer;

extern HINSTANCE hAppInstance;

void __cdecl OutputDebugStringF(const char* format, ...) {
	va_list vlArgs;
	char* strBuffer = (char*)GlobalAlloc(GPTR, 4096);

	va_start(vlArgs, format);
	_vsnprintf(strBuffer, 4096 - 1, format, vlArgs);
	va_end(vlArgs);
	//strcat(strBuffer, "\n");
	OutputDebugStringA(strBuffer);
	GlobalFree(strBuffer);
	return;
}

void __cdecl OutputDebugStringWF(const wchar_t* format, ...) {
	va_list vlArgs;
	wchar_t* strBuffer = (wchar_t*)GlobalAlloc(GPTR, 4096);

	va_start(vlArgs, format);
	_vsnwprintf(strBuffer, 4096 - 1, format, vlArgs);
	va_end(vlArgs);
	//strcat(strBuffer, "\n");
	OutputDebugStringW(strBuffer);
	GlobalFree(strBuffer);
	return;
}



__int64 FileSize;
__int64 newFileSize;

void FileToFileBuffer(CHAR* PATH, LPVOID* pFileBuffer) {
	FILE* fp = fopen(PATH, "rb");
	fseek(fp, 0, SEEK_END);
	FileSize = ftell(fp);
	fseek(fp, 0, 0);
	BYTE* FileBuffer = (BYTE*)malloc(FileSize);
	memset(FileBuffer, FileSize, 0);
	fread(FileBuffer, FileSize, 1, fp);
	*pFileBuffer = FileBuffer;
	fclose(fp);
	fp = NULL;

	return;
}



void FileBufferToImageBuffer(LPVOID pFileBuffer, LPVOID* pImageBuffer) {
	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)pFileBuffer;
	_IMAGE_NT_HEADERS* NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)pFileBuffer + DOS_HEADER->e_lfanew);
	_IMAGE_SECTION_HEADER* SECTION_HEADER = (_IMAGE_SECTION_HEADER*)((DWORD)pFileBuffer + DOS_HEADER->e_lfanew + 0x4 + sizeof(_IMAGE_FILE_HEADER) + NT_HEADERS->FileHeader.SizeOfOptionalHeader);
	BYTE* ImageBuffer = (BYTE*)malloc(NT_HEADERS->OptionalHeader.SizeOfImage);
	memset(ImageBuffer, NT_HEADERS->OptionalHeader.SizeOfImage, 0);
	*pImageBuffer = ImageBuffer;
	memcpy((BYTE*)ImageBuffer, (BYTE*)pFileBuffer, NT_HEADERS->OptionalHeader.SizeOfHeaders);
	for (int i = 0; i < NT_HEADERS->FileHeader.NumberOfSections; i++) {
		_IMAGE_SECTION_HEADER* SEC = SECTION_HEADER + i;
		memcpy((BYTE*)ImageBuffer + SEC->VirtualAddress, (BYTE*)pFileBuffer + SEC->PointerToRawData, SEC->SizeOfRawData);
	}
	return;
}


void ImageBufferToNewBuffer(LPVOID pImageBuffer, LPVOID* pNewBuffer) {
	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)pImageBuffer;
	_IMAGE_NT_HEADERS* NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)pImageBuffer + DOS_HEADER->e_lfanew);
	_IMAGE_SECTION_HEADER* SECTION_HEADER = (_IMAGE_SECTION_HEADER*)((DWORD)pImageBuffer + DOS_HEADER->e_lfanew + 0x4 + sizeof(_IMAGE_FILE_HEADER) + NT_HEADERS->FileHeader.SizeOfOptionalHeader);
	_IMAGE_SECTION_HEADER* LastSection = SECTION_HEADER + NT_HEADERS->FileHeader.NumberOfSections - 1;
	newFileSize = LastSection->PointerToRawData + LastSection->SizeOfRawData;
	BYTE* NewBuffer = (BYTE*)malloc(newFileSize);
	memset(NewBuffer, 0, newFileSize);
	*pNewBuffer = NewBuffer;
	memcpy((BYTE*)NewBuffer, (BYTE*)pImageBuffer, NT_HEADERS->OptionalHeader.SizeOfHeaders);
	for (int i = 0; i < NT_HEADERS->FileHeader.NumberOfSections; i++) {
		_IMAGE_SECTION_HEADER* SEC = SECTION_HEADER + i;
		memcpy((BYTE*)NewBuffer + SEC->PointerToRawData, (BYTE*)pImageBuffer + SEC->VirtualAddress, SEC->SizeOfRawData);
	}
	return;
}

void NewBufferToFile(LPVOID pNewBuffer, char* PATH) {
	FILE* fp = fopen(PATH, "wb");
	fwrite(pNewBuffer, FileSize, 1, fp);
	fclose(fp);
	fp = NULL;
	return;
}

void AddCode(LPVOID pImageBuffer, char* text) {
	BYTE CodeBuffer[] = { 0x6A,0x00,
						0x6A,0x00,
						0x6A,0x00,
						0x6A,0x00,
						0xE8,0x00,0x00,0x00,0x00,
						0xE9,0x00,0x00,0x00,0x00
	};
	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)pImageBuffer;
	_IMAGE_NT_HEADERS* NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)pImageBuffer + DOS_HEADER->e_lfanew);
	_IMAGE_SECTION_HEADER* SECTION_HEADER = (_IMAGE_SECTION_HEADER*)((DWORD)pImageBuffer + DOS_HEADER->e_lfanew + 0x4 + sizeof(_IMAGE_FILE_HEADER) + NT_HEADERS->FileHeader.SizeOfOptionalHeader);
	int offset = 0;
	for (int i = 0; i < NT_HEADERS->FileHeader.NumberOfSections; i++) {
		if (!strcmp((char*)(SECTION_HEADER + i), text)) {
			offset = i;
			break;
		}
	}
	DWORD num = MessageBoxAddress - (SECTION_HEADER + offset)->Misc.VirtualSize - (SECTION_HEADER + offset)->VirtualAddress - NT_HEADERS->OptionalHeader.ImageBase - 0x8 - 0x5;
	*(DWORD*)(&CodeBuffer[9]) = num;
	num = NT_HEADERS->OptionalHeader.AddressOfEntryPoint - (SECTION_HEADER + offset)->Misc.VirtualSize - (SECTION_HEADER + offset)->VirtualAddress - 0x8 - 0x5 - 0x5;
	NT_HEADERS->OptionalHeader.AddressOfEntryPoint = (SECTION_HEADER + offset)->Misc.VirtualSize + (SECTION_HEADER + offset)->VirtualAddress;
	*(DWORD*)(&CodeBuffer[9 + 5]) = num;
	memcpy((BYTE*)pImageBuffer + (SECTION_HEADER + offset)->Misc.VirtualSize + (SECTION_HEADER + offset)->VirtualAddress, CodeBuffer, sizeof(CodeBuffer));
	(SECTION_HEADER + offset)->Misc.VirtualSize += sizeof(CodeBuffer);

	(SECTION_HEADER + offset)->Characteristics |= 0x60000020;
}

void AddSectionToImageBuffer(LPVOID* pImageBuffer) {
	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)*pImageBuffer;
	_IMAGE_NT_HEADERS* NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)*pImageBuffer + DOS_HEADER->e_lfanew);
	_IMAGE_SECTION_HEADER* SECTION_HEADER = (_IMAGE_SECTION_HEADER*)((DWORD)*pImageBuffer + DOS_HEADER->e_lfanew + 0x4 + sizeof(_IMAGE_FILE_HEADER) + NT_HEADERS->FileHeader.SizeOfOptionalHeader);
	if (NT_HEADERS->OptionalHeader.SizeOfHeaders -
		(DWORD)*pImageBuffer + DOS_HEADER->e_lfanew + 0x4 + sizeof(_IMAGE_FILE_HEADER) + NT_HEADERS->FileHeader.SizeOfOptionalHeader +
		sizeof(_IMAGE_SECTION_HEADER) * (NT_HEADERS->FileHeader.NumberOfSections) < 80) {
		printf("cant add a new seciton information!\n%d",
			NT_HEADERS->OptionalHeader.SizeOfHeaders -
			(DWORD)*pImageBuffer + DOS_HEADER->e_lfanew + 0x4 + sizeof(_IMAGE_FILE_HEADER) + NT_HEADERS->FileHeader.SizeOfOptionalHeader +
			sizeof(_IMAGE_SECTION_HEADER) * (NT_HEADERS->FileHeader.NumberOfSections));
		return;
	}
	DWORD size = NT_HEADERS->OptionalHeader.SizeOfImage;
	_IMAGE_SECTION_HEADER NEW_SECTION = {};
	memcpy(&NEW_SECTION.Name, ".hello", 0x8);
	//NEW_SECTION.Misc.VirtualSize=0x1000;
	NEW_SECTION.VirtualAddress = NT_HEADERS->OptionalHeader.SizeOfImage;
	NEW_SECTION.SizeOfRawData = NT_HEADERS->OptionalHeader.FileAlignment;
	NEW_SECTION.PointerToRawData = (SECTION_HEADER + NT_HEADERS->FileHeader.NumberOfSections - 1)->PointerToRawData + (SECTION_HEADER + NT_HEADERS->FileHeader.NumberOfSections - 1)->SizeOfRawData;
	NEW_SECTION.Characteristics |= 0x60000020;
	memcpy(SECTION_HEADER + NT_HEADERS->FileHeader.NumberOfSections, &NEW_SECTION, sizeof(_IMAGE_SECTION_HEADER));
	NT_HEADERS->OptionalHeader.SizeOfImage += NT_HEADERS->OptionalHeader.SectionAlignment;
	NT_HEADERS->FileHeader.NumberOfSections++;
	BYTE* NewImageBuffer = (BYTE*)malloc(NT_HEADERS->OptionalHeader.SizeOfImage);
	memset(NewImageBuffer, 0, NT_HEADERS->OptionalHeader.SizeOfImage);
	memcpy(NewImageBuffer, *pImageBuffer, size);

	free(*pImageBuffer);
	*pImageBuffer = NewImageBuffer;
	//FileSize+=0x1000;
	return;
}
void ShiftSectionHeader(LPVOID pFileBuffer) {
	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)pFileBuffer;
	_IMAGE_NT_HEADERS* NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)pFileBuffer + DOS_HEADER->e_lfanew);
	_IMAGE_SECTION_HEADER* SECTION_HEADER = (_IMAGE_SECTION_HEADER*)((DWORD)pFileBuffer + DOS_HEADER->e_lfanew + 0x4 + sizeof(_IMAGE_FILE_HEADER) + NT_HEADERS->FileHeader.SizeOfOptionalHeader);
	int offset = DOS_HEADER->e_lfanew - sizeof(_IMAGE_DOS_HEADER);
	DOS_HEADER->e_lfanew = sizeof(_IMAGE_DOS_HEADER);

	memcpy((BYTE*)pFileBuffer + sizeof(_IMAGE_DOS_HEADER), NT_HEADERS, DOS_HEADER->e_lfanew + 0x4 + sizeof(_IMAGE_FILE_HEADER) + NT_HEADERS->FileHeader.SizeOfOptionalHeader + sizeof(_IMAGE_SECTION_HEADER) * NT_HEADERS->FileHeader.NumberOfSections);
	NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)pFileBuffer + DOS_HEADER->e_lfanew);
	memset((BYTE*)pFileBuffer + sizeof(_IMAGE_DOS_HEADER) + 0x4 + sizeof(_IMAGE_FILE_HEADER) + NT_HEADERS->FileHeader.SizeOfOptionalHeader + sizeof(_IMAGE_SECTION_HEADER) * NT_HEADERS->FileHeader.NumberOfSections, 0, offset);
	return;
}



int Alignment(int num, int AlignmentNum) {
	if (num == 0) return 0;
	return ((num - 1) / AlignmentNum + 1) * AlignmentNum;
}

//void ExLastSection(LPVOID pFileBuffer){
//	_IMAGE_DOS_HEADER* DOS_HEADER=(_IMAGE_DOS_HEADER*)pFileBuffer;
//	_IMAGE_NT_HEADERS* NT_HEADERS=(_IMAGE_NT_HEADERS*)((DWORD)pFileBuffer+DOS_HEADER->e_lfanew);
//	_IMAGE_SECTION_HEADER* SECTION_HEADER=(_IMAGE_SECTION_HEADER*)((DWORD)pFileBuffer+DOS_HEADER->e_lfanew+0x4+sizeof(_IMAGE_FILE_HEADER)+NT_HEADERS->FileHeader.SizeOfOptionalHeader);
//	
//	_IMAGE_SECTION_HEADER* LastSection=SECTION_HEADER+NT_HEADERS->FileHeader.NumberOfSections-1;
//	//LastSection->Misc.VirtualSize+=0x1000;
//	LastSection->SizeOfRawData+=0x1000;
//	NT_HEADERS->OptionalHeader.SizeOfImage+=0x1000;
//	//FileSize+=0x1000;
//	return;
//}


void ExLastSectionToImageBuffer(LPVOID* pImageBuffer) {
	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)*pImageBuffer;
	_IMAGE_NT_HEADERS* NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)*pImageBuffer + DOS_HEADER->e_lfanew);
	_IMAGE_SECTION_HEADER* SECTION_HEADER = (_IMAGE_SECTION_HEADER*)((DWORD)*pImageBuffer + DOS_HEADER->e_lfanew + 0x4 + sizeof(_IMAGE_FILE_HEADER) + NT_HEADERS->FileHeader.SizeOfOptionalHeader);
	_IMAGE_SECTION_HEADER* LastSection = SECTION_HEADER - 1 + NT_HEADERS->FileHeader.NumberOfSections;
	DWORD size = NT_HEADERS->OptionalHeader.SizeOfImage;
	NT_HEADERS->OptionalHeader.SizeOfImage += 0x1000;
	LastSection->SizeOfRawData += 0x1000;

	BYTE* NewImageBuffer = (BYTE*)malloc(NT_HEADERS->OptionalHeader.SizeOfImage);
	memset(NewImageBuffer, 0, NT_HEADERS->OptionalHeader.SizeOfImage);
	memcpy(NewImageBuffer, *pImageBuffer, size);
	free(*pImageBuffer);
	*pImageBuffer = NewImageBuffer;
	LastSection->Characteristics |= 0x60000020;
	return;
}


void PrintAll(LPVOID pImageBuffer) {
	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)pImageBuffer;
	_IMAGE_NT_HEADERS* NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)pImageBuffer + DOS_HEADER->e_lfanew);
	_IMAGE_SECTION_HEADER* SECTION_HEADER = (_IMAGE_SECTION_HEADER*)((DWORD)pImageBuffer + DOS_HEADER->e_lfanew + 0x4 + sizeof(_IMAGE_FILE_HEADER) + NT_HEADERS->FileHeader.SizeOfOptionalHeader);
	printf("导出表结构\n%x\n%x\n", NT_HEADERS->OptionalHeader.DataDirectory[0].VirtualAddress, NT_HEADERS->OptionalHeader.DataDirectory[0].Size);
	printf("导入表结构\n%x\n%x\n", NT_HEADERS->OptionalHeader.DataDirectory[1].VirtualAddress, NT_HEADERS->OptionalHeader.DataDirectory[1].Size);
	printf("资源表结构\n%x\n%x\n", NT_HEADERS->OptionalHeader.DataDirectory[2].VirtualAddress, NT_HEADERS->OptionalHeader.DataDirectory[2].Size);
	printf("例外表结构\n%x\n%x\n", NT_HEADERS->OptionalHeader.DataDirectory[3].VirtualAddress, NT_HEADERS->OptionalHeader.DataDirectory[3].Size);
	printf("安全表结构\n%x\n%x\n", NT_HEADERS->OptionalHeader.DataDirectory[4].VirtualAddress, NT_HEADERS->OptionalHeader.DataDirectory[4].Size);
	printf("重定向表结构\n%x\n%x\n", NT_HEADERS->OptionalHeader.DataDirectory[5].VirtualAddress, NT_HEADERS->OptionalHeader.DataDirectory[5].Size);
	printf("调试表结构\n%x\n%x\n", NT_HEADERS->OptionalHeader.DataDirectory[6].VirtualAddress, NT_HEADERS->OptionalHeader.DataDirectory[6].Size);
	printf("版权结构\n%x\n%x\n", NT_HEADERS->OptionalHeader.DataDirectory[7].VirtualAddress, NT_HEADERS->OptionalHeader.DataDirectory[7].Size);
	printf("全局指针表结构\n%x\n%x\n", NT_HEADERS->OptionalHeader.DataDirectory[8].VirtualAddress, NT_HEADERS->OptionalHeader.DataDirectory[8].Size);
	printf("Tls表结构\n%x\n%x\n", NT_HEADERS->OptionalHeader.DataDirectory[9].VirtualAddress, NT_HEADERS->OptionalHeader.DataDirectory[9].Size);
	printf("载入配置表结构\n%x\n%x\n", NT_HEADERS->OptionalHeader.DataDirectory[10].VirtualAddress, NT_HEADERS->OptionalHeader.DataDirectory[10].Size);
	printf("输入范围表结构\n%x\n%x\n", NT_HEADERS->OptionalHeader.DataDirectory[11].VirtualAddress, NT_HEADERS->OptionalHeader.DataDirectory[11].Size);
	printf("IAT表结构\n%x\n%x\n", NT_HEADERS->OptionalHeader.DataDirectory[12].VirtualAddress, NT_HEADERS->OptionalHeader.DataDirectory[12].Size);
	printf("延迟输入表结构\n%x\n%x\n", NT_HEADERS->OptionalHeader.DataDirectory[13].VirtualAddress, NT_HEADERS->OptionalHeader.DataDirectory[13].Size);
	printf("COM表结构\n%x\n%x\n", NT_HEADERS->OptionalHeader.DataDirectory[14].VirtualAddress, NT_HEADERS->OptionalHeader.DataDirectory[14].Size);
	printf("保留表结构\n%x\n%x\n", NT_HEADERS->OptionalHeader.DataDirectory[15].VirtualAddress, NT_HEADERS->OptionalHeader.DataDirectory[15].Size);
}

void SumSection(LPVOID pImageBuffer) {
	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)pImageBuffer;
	_IMAGE_NT_HEADERS* NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)pImageBuffer + DOS_HEADER->e_lfanew);
	_IMAGE_SECTION_HEADER* SECTION_HEADER = (_IMAGE_SECTION_HEADER*)((DWORD)pImageBuffer + DOS_HEADER->e_lfanew + 0x4 + sizeof(_IMAGE_FILE_HEADER) + NT_HEADERS->FileHeader.SizeOfOptionalHeader);
	SECTION_HEADER += 1;
	SECTION_HEADER->SizeOfRawData = NT_HEADERS->OptionalHeader.SizeOfImage - SECTION_HEADER->VirtualAddress;
	SECTION_HEADER->Misc.VirtualSize = SECTION_HEADER->SizeOfRawData;
	for (int i = 0; i < NT_HEADERS->FileHeader.NumberOfSections; i++) {
		SECTION_HEADER->Characteristics |= (SECTION_HEADER + i)->Characteristics;
	}
	NT_HEADERS->FileHeader.NumberOfSections = 2;
	return;
}

DWORD RVATOFOA(LPVOID pFileBuffer, DWORD RVA) {

	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)pFileBuffer;
	_IMAGE_NT_HEADERS* NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)pFileBuffer + DOS_HEADER->e_lfanew);
	_IMAGE_SECTION_HEADER* SECTION_HEADER = (_IMAGE_SECTION_HEADER*)((DWORD)pFileBuffer + DOS_HEADER->e_lfanew + 0x4 + sizeof(_IMAGE_FILE_HEADER) + NT_HEADERS->FileHeader.SizeOfOptionalHeader);
	if (RVA < SECTION_HEADER->VirtualAddress)return RVA;
	for (int i = 0; i < NT_HEADERS->FileHeader.NumberOfSections; i++) {
		if (i == NT_HEADERS->FileHeader.NumberOfSections - 1 || (RVA >= (SECTION_HEADER + i)->VirtualAddress && RVA < (SECTION_HEADER + i + 1)->VirtualAddress))
			return RVA + (SECTION_HEADER + i)->PointerToRawData - (SECTION_HEADER + i)->VirtualAddress;
	}
}

DWORD FOATORVA(LPVOID pFileBuffer, DWORD FOA) {
	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)pFileBuffer;
	_IMAGE_NT_HEADERS* NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)pFileBuffer + DOS_HEADER->e_lfanew);
	_IMAGE_SECTION_HEADER* SECTION_HEADER = (_IMAGE_SECTION_HEADER*)((DWORD)pFileBuffer + DOS_HEADER->e_lfanew + 0x4 + sizeof(_IMAGE_FILE_HEADER) + NT_HEADERS->FileHeader.SizeOfOptionalHeader);
	if (FOA < SECTION_HEADER->PointerToRawData)return FOA;
	for (int i = 0; i < NT_HEADERS->FileHeader.NumberOfSections; i++) {
		if (i == NT_HEADERS->FileHeader.NumberOfSections - 1 || (FOA >= (SECTION_HEADER + i)->PointerToRawData && FOA < (SECTION_HEADER + i + 1)->PointerToRawData))
			return FOA - (SECTION_HEADER + i)->PointerToRawData + (SECTION_HEADER + i)->VirtualAddress;
	}
}


void ExportTablePrint(LPVOID pFileBuffer) {
	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)pFileBuffer;
	_IMAGE_NT_HEADERS* NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)pFileBuffer + DOS_HEADER->e_lfanew);
	_IMAGE_EXPORT_DIRECTORY* EXPORT_DIR = (_IMAGE_EXPORT_DIRECTORY*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, NT_HEADERS->OptionalHeader.DataDirectory[0].VirtualAddress));
	for (int i = 0; i < EXPORT_DIR->NumberOfFunctions; i++) {
		printf("%x\n", *(DWORD*)((BYTE*)pFileBuffer + RVATOFOA(pFileBuffer, (DWORD)((DWORD*)EXPORT_DIR->AddressOfFunctions + i))));
	}
	for (int i = 0; i < EXPORT_DIR->NumberOfNames; i++) {
		printf("%x\n", *(WORD*)((BYTE*)pFileBuffer + RVATOFOA(pFileBuffer, (DWORD)((WORD*)EXPORT_DIR->AddressOfNameOrdinals + i))));
	}
	for (int i = 0; i < EXPORT_DIR->NumberOfNames; i++) {
		printf("%s\n", (DWORD)pFileBuffer + RVATOFOA(pFileBuffer, *(DWORD*)((BYTE*)pFileBuffer + RVATOFOA(pFileBuffer, ((DWORD)((DWORD*)EXPORT_DIR->AddressOfNames + i))))));
	}

}
DWORD GetFunctionAddrByName(LPVOID pFileBuffer, char* text) {

	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)pFileBuffer;
	_IMAGE_NT_HEADERS* NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)pFileBuffer + DOS_HEADER->e_lfanew);
	_IMAGE_EXPORT_DIRECTORY* EXPORT_DIR = (_IMAGE_EXPORT_DIRECTORY*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, NT_HEADERS->OptionalHeader.DataDirectory[0].VirtualAddress));
	for (int i = 0; i < EXPORT_DIR->NumberOfNames; i++) {
		if (!strcmp((char*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, *(DWORD*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, (DWORD)((DWORD*)EXPORT_DIR->AddressOfNames + i))))), text)) {
			int offset = *(WORD*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, (DWORD)((WORD*)EXPORT_DIR->AddressOfNameOrdinals + i)));
			printf("%s  ->  %x\n", ((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, *(DWORD*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, (DWORD)((DWORD*)EXPORT_DIR->AddressOfNames + i))))), *(DWORD*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, (DWORD)((DWORD*)EXPORT_DIR->AddressOfFunctions + offset))));
			return *(DWORD*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, (DWORD)((DWORD*)EXPORT_DIR->AddressOfFunctions + offset)));
		}

	}
}
DWORD GetFunctionAddrByOrdinals(LPVOID pFileBuffer, DWORD NUM) {
	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)pFileBuffer;
	_IMAGE_NT_HEADERS* NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)pFileBuffer + DOS_HEADER->e_lfanew);
	_IMAGE_EXPORT_DIRECTORY* EXPORT_DIR = (_IMAGE_EXPORT_DIRECTORY*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, NT_HEADERS->OptionalHeader.DataDirectory[0].VirtualAddress));
	NUM -= EXPORT_DIR->Base;
	printf("%x\n", *(DWORD*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, (DWORD)((DWORD*)EXPORT_DIR->AddressOfFunctions + NUM))));
	return *(DWORD*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, (DWORD)((DWORD*)EXPORT_DIR->AddressOfFunctions + NUM)));
}


void PrintBaseRelocTableAll(LPVOID pFileBuffer) {
	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)pFileBuffer;
	_IMAGE_NT_HEADERS* NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)pFileBuffer + DOS_HEADER->e_lfanew);
	_IMAGE_BASE_RELOCATION* BASE_RELOC = (_IMAGE_BASE_RELOCATION*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, NT_HEADERS->OptionalHeader.DataDirectory[5].VirtualAddress));

	while (BASE_RELOC->VirtualAddress || BASE_RELOC->SizeOfBlock) {
		int NUM = (BASE_RELOC->SizeOfBlock - 0x8) / 2;
		int RVA;
		for (int i = 0; i < NUM; i++) {
			if ((*((WORD*)((DWORD)BASE_RELOC + 0x8) + i)) >> 12 == 0x3) {
				RVA = BASE_RELOC->VirtualAddress + ((*((WORD*)((DWORD)BASE_RELOC + 0x8) + i)) & 0x0FFF);
				printf("%d     RVA=%x   FOA=%x\n", i + 1, RVA, RVATOFOA(pFileBuffer, RVA));
			}
		}
		printf("****************\n");
		BASE_RELOC = (_IMAGE_BASE_RELOCATION*)((DWORD)BASE_RELOC + BASE_RELOC->SizeOfBlock);
	}
}


void MoveTableToNewSection(LPVOID pFileBuffer, DWORD FOA) {
	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)pFileBuffer;
	_IMAGE_NT_HEADERS* NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)pFileBuffer + DOS_HEADER->e_lfanew);
	_IMAGE_EXPORT_DIRECTORY* EXPORT_DIR = (_IMAGE_EXPORT_DIRECTORY*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, NT_HEADERS->OptionalHeader.DataDirectory[0].VirtualAddress));
	DWORD start = 0;
	DWORD NewSectionStart = (DWORD)pFileBuffer + FOA;
	memcpy((BYTE*)NewSectionStart + start, (BYTE*)pFileBuffer + RVATOFOA(pFileBuffer, EXPORT_DIR->AddressOfFunctions), EXPORT_DIR->NumberOfFunctions * 4);
	EXPORT_DIR->AddressOfFunctions = FOATORVA(pFileBuffer, FOA + start);
	start += EXPORT_DIR->NumberOfFunctions * 4;
	memcpy((BYTE*)NewSectionStart + start, (BYTE*)pFileBuffer + RVATOFOA(pFileBuffer, EXPORT_DIR->AddressOfNameOrdinals), EXPORT_DIR->NumberOfNames * 2);
	EXPORT_DIR->AddressOfNameOrdinals = FOATORVA(pFileBuffer, FOA + start);
	start += EXPORT_DIR->NumberOfNames * 2;
	int cnt = 0;
	while (cnt < EXPORT_DIR->NumberOfNames) {
		memcpy((BYTE*)NewSectionStart + start, (char*)((BYTE*)pFileBuffer + RVATOFOA(pFileBuffer, *(DWORD*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, (DWORD)((DWORD*)EXPORT_DIR->AddressOfNames + cnt))))), strlen((char*)((BYTE*)pFileBuffer + RVATOFOA(pFileBuffer, *(DWORD*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, (DWORD)((DWORD*)EXPORT_DIR->AddressOfNames + cnt)))))) + 1);
		int laststart = start;
		start += strlen((char*)((BYTE*)pFileBuffer + RVATOFOA(pFileBuffer, *(DWORD*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, (DWORD)((DWORD*)EXPORT_DIR->AddressOfNames + cnt)))))) + 1;
		*(DWORD*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, (DWORD)((DWORD*)EXPORT_DIR->AddressOfNames + cnt))) = FOATORVA(pFileBuffer, FOA + laststart);
		cnt++;
	}
	memcpy((BYTE*)NewSectionStart + start, (BYTE*)pFileBuffer + RVATOFOA(pFileBuffer, EXPORT_DIR->AddressOfNames), EXPORT_DIR->NumberOfNames * 4);
	EXPORT_DIR->AddressOfNames = FOATORVA(pFileBuffer, FOA + start);
	start += EXPORT_DIR->NumberOfNames * 4;
	memcpy((BYTE*)NewSectionStart + start, EXPORT_DIR, sizeof(_IMAGE_EXPORT_DIRECTORY));
	NT_HEADERS->OptionalHeader.DataDirectory[0].VirtualAddress = FOATORVA(pFileBuffer, FOA + start);
	start += sizeof(_IMAGE_EXPORT_DIRECTORY);
	_IMAGE_BASE_RELOCATION* BASE_RELOC = (_IMAGE_BASE_RELOCATION*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, NT_HEADERS->OptionalHeader.DataDirectory[5].VirtualAddress));
	int sum = 0;
	while (BASE_RELOC->SizeOfBlock || BASE_RELOC->VirtualAddress) {
		memcpy((BYTE*)NewSectionStart + start, BASE_RELOC, BASE_RELOC->SizeOfBlock);
		sum += BASE_RELOC->SizeOfBlock;
		start += BASE_RELOC->SizeOfBlock;
		BASE_RELOC = (_IMAGE_BASE_RELOCATION*)((DWORD)BASE_RELOC + BASE_RELOC->SizeOfBlock);
	}
	NT_HEADERS->OptionalHeader.DataDirectory[5].VirtualAddress = FOATORVA(pFileBuffer, FOA + start - sum);

}
DWORD AddSectionToFileBuffer(LPVOID* pFileBuffer, char* text, DWORD SizeOfSection) {
	DWORD LastFileSize = FileSize;
	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)*pFileBuffer;
	_IMAGE_NT_HEADERS* NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)*pFileBuffer + DOS_HEADER->e_lfanew);
	_IMAGE_SECTION_HEADER* SECTION_HEADER = (_IMAGE_SECTION_HEADER*)((DWORD)*pFileBuffer + DOS_HEADER->e_lfanew + 0x4 + sizeof(_IMAGE_FILE_HEADER) + NT_HEADERS->FileHeader.SizeOfOptionalHeader);
	_IMAGE_SECTION_HEADER NewSection = {};
	memcpy(&NewSection.Name, text, 0x8);
	NewSection.VirtualAddress = NT_HEADERS->OptionalHeader.SizeOfImage;
	NewSection.SizeOfRawData = SizeOfSection;
	NewSection.PointerToRawData = Alignment(LastFileSize, NT_HEADERS->OptionalHeader.FileAlignment);
	NewSection.Characteristics |= 0xC0000040;
	NT_HEADERS->OptionalHeader.SizeOfImage += Alignment(SizeOfSection, NT_HEADERS->OptionalHeader.SectionAlignment);

	FileSize = Alignment(FileSize + SizeOfSection, NT_HEADERS->OptionalHeader.FileAlignment);
	VOID* FileBuffer = (VOID*)malloc(FileSize);
	memset(FileBuffer, 0, FileSize);

	memcpy(SECTION_HEADER + NT_HEADERS->FileHeader.NumberOfSections, &NewSection, sizeof(_IMAGE_SECTION_HEADER));
	NT_HEADERS->FileHeader.NumberOfSections++;
	memcpy(FileBuffer, *pFileBuffer, LastFileSize);
	free(*pFileBuffer);
	*pFileBuffer = FileBuffer;
	return LastFileSize;

}
void ChangeImageBase(LPVOID pFileBuffer, DWORD ImageBase) {
	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)pFileBuffer;
	_IMAGE_NT_HEADERS* NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)pFileBuffer + DOS_HEADER->e_lfanew);
	_IMAGE_BASE_RELOCATION* BASE_RELOC = (_IMAGE_BASE_RELOCATION*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, NT_HEADERS->OptionalHeader.DataDirectory[5].VirtualAddress));
	DWORD offset = ImageBase - NT_HEADERS->OptionalHeader.ImageBase;
	while (BASE_RELOC->SizeOfBlock || BASE_RELOC->VirtualAddress) {
		int NUM = (BASE_RELOC->SizeOfBlock - 0x8) / 2;
		for (int i = 0; i < NUM; i++) {
			if ((*((WORD*)((DWORD)BASE_RELOC + 0x8) + i) >> 12) == 3) {
				*(DWORD*)((DWORD)pFileBuffer + (RVATOFOA(pFileBuffer, BASE_RELOC->VirtualAddress + (*((WORD*)((DWORD)BASE_RELOC + 0x8) + i) & 0x0FFF)))) += offset;
			}
		}
		BASE_RELOC = (_IMAGE_BASE_RELOCATION*)((DWORD)BASE_RELOC + BASE_RELOC->SizeOfBlock);
	}
	NT_HEADERS->OptionalHeader.ImageBase = ImageBase;
}


void PrintImportTable(LPVOID pFileBuffer) {
	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)pFileBuffer;
	_IMAGE_NT_HEADERS* NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)pFileBuffer + DOS_HEADER->e_lfanew);
	_IMAGE_IMPORT_DESCRIPTOR* IMPORT_TABLE = (_IMAGE_IMPORT_DESCRIPTOR*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, NT_HEADERS->OptionalHeader.DataDirectory[1].VirtualAddress));
	DWORD offset1 = 0;
	DWORD offset2 = 0;
	while ((IMPORT_TABLE + offset1)->Characteristics || (IMPORT_TABLE + offset1)->FirstThunk || (IMPORT_TABLE + offset1)->ForwarderChain || (IMPORT_TABLE + offset1)->Name || (IMPORT_TABLE + offset1)->OriginalFirstThunk || (IMPORT_TABLE + offset1)->TimeDateStamp) {
		printf("%s\n", (DWORD)pFileBuffer + RVATOFOA(pFileBuffer, (IMPORT_TABLE + offset1)->Name));
		_IMAGE_THUNK_DATA32* INT_ = (_IMAGE_THUNK_DATA32*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, (IMPORT_TABLE + offset1)->OriginalFirstThunk));
		printf("INT_TABLE:%x--->%x\n", INT_, *INT_);
		while ((INT_ + offset2)->u1.Ordinal) {
			if ((INT_ + offset2)->u1.Ordinal & 0x80000000) {
				printf("%d按序号导入:%d\n", offset2 + 1, (INT_ + offset2)->u1.Ordinal & 0x7FFFFFFFFF);
			}
			else {
				_IMAGE_IMPORT_BY_NAME* IMPORT_BY_NAME = (_IMAGE_IMPORT_BY_NAME*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, (INT_ + offset2)->u1.Ordinal));
				printf("%d按名字导入:%x---->%s\n", offset2 + 1, IMPORT_BY_NAME->Hint, IMPORT_BY_NAME->Name);
			}
			offset2++;
		}
		puts("----------------------------");

		offset2 = 0;
		_IMAGE_THUNK_DATA32* IAT_ = (_IMAGE_THUNK_DATA32*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, (IMPORT_TABLE + offset1)->FirstThunk));
		printf("IAT_TABLE:%x--->%x\n", IAT_, *IAT_);
		while ((IAT_ + offset2)->u1.Ordinal) {
			if ((IAT_ + offset2)->u1.Ordinal & 0x80000000) {
				printf("%d按序号导入:%d\n", offset2 + 1, (IAT_ + offset2)->u1.Ordinal & 0x7FFFFFFF);
			}
			else {
				_IMAGE_IMPORT_BY_NAME* IMPORT_BY_NAME = (_IMAGE_IMPORT_BY_NAME*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, (IAT_ + offset2)->u1.Ordinal));
				printf("%d按名字导入:%x---->%s\n", offset2 + 1, IMPORT_BY_NAME->Hint, IMPORT_BY_NAME->Name);
			}
			offset2++;
		}
		offset2 = 0;
		puts("----------------------------");
		offset1++;
	}

}

void PrintBoundImportTable(LPVOID pFileBuffer) {
	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)pFileBuffer;
	_IMAGE_NT_HEADERS* NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)pFileBuffer + DOS_HEADER->e_lfanew);
	_IMAGE_BOUND_IMPORT_DESCRIPTOR* BOUND_IMPORT_TABLE = (_IMAGE_BOUND_IMPORT_DESCRIPTOR*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, NT_HEADERS->OptionalHeader.DataDirectory[11].VirtualAddress));
	DWORD COUNT = 0;
	DWORD OFFSET = 0;
	while ((BOUND_IMPORT_TABLE + OFFSET)->TimeDateStamp || (BOUND_IMPORT_TABLE + OFFSET)->OffsetModuleName || (BOUND_IMPORT_TABLE + OFFSET)->NumberOfModuleForwarderRefs) {
		COUNT = (BOUND_IMPORT_TABLE + OFFSET)->NumberOfModuleForwarderRefs;
		printf("%x---%s\n", (BOUND_IMPORT_TABLE + OFFSET)->TimeDateStamp, (DWORD)BOUND_IMPORT_TABLE + (BOUND_IMPORT_TABLE + OFFSET)->OffsetModuleName);
		while (COUNT--) {
			OFFSET++;
			printf("%x---%s\n", (BOUND_IMPORT_TABLE + OFFSET)->TimeDateStamp, (DWORD)BOUND_IMPORT_TABLE + (BOUND_IMPORT_TABLE + OFFSET)->OffsetModuleName);
		}
		OFFSET++;
	}
}


void InjectImportTable(LPVOID pFileBuffer, char* SectionName, char* FunctionName, char* dllName) {
	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)pFileBuffer;
	_IMAGE_NT_HEADERS* NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)pFileBuffer + DOS_HEADER->e_lfanew);
	_IMAGE_SECTION_HEADER* SECTION_HEADER = (_IMAGE_SECTION_HEADER*)((DWORD)pFileBuffer + DOS_HEADER->e_lfanew + 0x4 + sizeof(_IMAGE_FILE_HEADER) + NT_HEADERS->FileHeader.SizeOfOptionalHeader);
	_IMAGE_IMPORT_DESCRIPTOR* IMPORT_TABLE = (_IMAGE_IMPORT_DESCRIPTOR*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, NT_HEADERS->OptionalHeader.DataDirectory[1].VirtualAddress));
	DWORD SECTION_OFFSET = -1;
	for (int i = 0; i < NT_HEADERS->FileHeader.NumberOfSections; i++) {
		if (!strcmp(SectionName, (char*)(SECTION_HEADER + i)->Name)) {
			SECTION_OFFSET = i;
		}
	}
	if (SECTION_OFFSET == -1) {
		printf("未找到指定段\n");
		return;
	}
	DWORD SectionStart = (DWORD)pFileBuffer + (SECTION_HEADER + SECTION_OFFSET)->Misc.VirtualSize + (SECTION_HEADER + SECTION_OFFSET)->PointerToRawData;
	DWORD IMPORT_OFFSET = 0;
	DWORD start = 0;


	_IMAGE_IMPORT_DESCRIPTOR NewImport = {};
	memset((BYTE*)SectionStart + start, 0, 2);
	start += 0x2;
	memcpy((BYTE*)SectionStart + start, FunctionName, strlen(FunctionName) + 1);
	start += strlen(FunctionName) + 1;

	*(DWORD*)((DWORD)SectionStart + start) = FOATORVA(pFileBuffer, (DWORD)SectionStart + start - (DWORD)pFileBuffer - 0x3 - strlen(FunctionName));
	*(DWORD*)((DWORD)SectionStart + start + 0x4) = 0x00000000;
	DWORD _INT = (DWORD)SectionStart + start - (DWORD)pFileBuffer;
	start += 0x8;
	memcpy((BYTE*)SectionStart + start, dllName, strlen(dllName) + 1);
	NewImport.Name = FOATORVA(pFileBuffer, SectionStart + start - (DWORD)pFileBuffer);
	start += strlen(dllName) + 1;


	NewImport.OriginalFirstThunk = FOATORVA(pFileBuffer, _INT);
	NewImport.FirstThunk = FOATORVA(pFileBuffer, _INT);
	NT_HEADERS->OptionalHeader.DataDirectory[1].VirtualAddress = FOATORVA(pFileBuffer, (DWORD)SectionStart + start - (DWORD)pFileBuffer);
	while ((IMPORT_TABLE + IMPORT_OFFSET)->FirstThunk || (IMPORT_TABLE + IMPORT_OFFSET)->ForwarderChain || (IMPORT_TABLE + IMPORT_OFFSET)->Name || (IMPORT_TABLE + IMPORT_OFFSET)->OriginalFirstThunk || (IMPORT_TABLE + IMPORT_OFFSET)->TimeDateStamp) {
		memcpy((BYTE*)SectionStart + start, IMPORT_TABLE + IMPORT_OFFSET, sizeof(_IMAGE_IMPORT_DESCRIPTOR));
		start += sizeof(_IMAGE_IMPORT_DESCRIPTOR);
		IMPORT_OFFSET++;
	}
	memcpy((BYTE*)SectionStart + start, &NewImport, sizeof(_IMAGE_IMPORT_DESCRIPTOR));
	start += sizeof(_IMAGE_IMPORT_DESCRIPTOR);
	memset((BYTE*)SectionStart + start, 0, sizeof(_IMAGE_IMPORT_DESCRIPTOR));
	start += sizeof(_IMAGE_IMPORT_DESCRIPTOR);




	//NT_HEADERS->OptionalHeader.DataDirectory[1].Size += 0x14;
	(SECTION_HEADER + SECTION_OFFSET)->Misc.VirtualSize = start;

}
// TODO: 在 STDAFX.H 中
// 引用任何所需的附加头文件，而不是在此文件中引用
void ResourceTablePrint(LPVOID pFileBuffer) {
	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)pFileBuffer;
	_IMAGE_NT_HEADERS* NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)pFileBuffer + DOS_HEADER->e_lfanew);
	DWORD Base = (DWORD)pFileBuffer + RVATOFOA(pFileBuffer, NT_HEADERS->OptionalHeader.DataDirectory[2].VirtualAddress);
	ResourceTablePrintDFS(pFileBuffer, Base, 0, Base);
}
void ResourceTablePrintDFS(LPVOID pFileBuffer, DWORD OFFSET, DWORD lay, DWORD Base) {
	_IMAGE_RESOURCE_DIRECTORY* RESOURCE_DIR = (_IMAGE_RESOURCE_DIRECTORY*)OFFSET;
	_IMAGE_RESOURCE_DIRECTORY_ENTRY* RESOURCE_DIR_E_START = (_IMAGE_RESOURCE_DIRECTORY_ENTRY*)((DWORD)RESOURCE_DIR + sizeof(_IMAGE_RESOURCE_DIRECTORY));
	for (int i = 0; i < RESOURCE_DIR->NumberOfIdEntries + RESOURCE_DIR->NumberOfNamedEntries; i++) {
		if ((RESOURCE_DIR_E_START + i)->NameIsString) {
			WCHAR wstr[40] = {};
			_IMAGE_RESOURCE_DIR_STRING_U* RESOURCE_U = (_IMAGE_RESOURCE_DIR_STRING_U*)((DWORD)Base + RVATOFOA(pFileBuffer, (RESOURCE_DIR_E_START + i)->NameOffset));
			memcpy(wstr, RESOURCE_U->NameString, RESOURCE_U->Length * sizeof(WCHAR));
			//printf("名字为%s\n", (DWORD)pFileBuffer + RVATOFOA(pFileBuffer, (RESOURCE_DIR_E_START+i)->NameOffset));
			switch (lay) {
			case 0:
				wprintf(L"名字为:%s\n", wstr);
				break;
			case 1:
				wprintf(L"  -->名字为: % s\n", wstr);
				break;
			case 2:
				wprintf(L"    -->名字为: % s\n", wstr);
				break;
			}

		}
		else {
			switch (lay) {
			case 0:
				wprintf(L"id为:%d\n", (RESOURCE_DIR_E_START + i)->Id);
				break;
			case 1:
				wprintf(L"  -->编号为:%d\n", (RESOURCE_DIR_E_START + i)->Id);
				break;
			case 2:
				wprintf(L"    -->代码为:%d\n", (RESOURCE_DIR_E_START + i)->Id);
				break;
			}
		}
		if ((RESOURCE_DIR_E_START + i)->DataIsDirectory) {
			ResourceTablePrintDFS(pFileBuffer, (DWORD)Base + RVATOFOA(pFileBuffer, (RESOURCE_DIR_E_START + i)->OffsetToDirectory), lay + 1, Base);
		}
		else {
			_IMAGE_DATA_DIRECTORY* DATA_DIR = (_IMAGE_DATA_DIRECTORY*)((DWORD)Base + RVATOFOA(pFileBuffer, (RESOURCE_DIR_E_START + i)->OffsetToDirectory));
			_IMAGE_RESOURCE_DATA_ENTRY* RESOURCE_DATA_E = (_IMAGE_RESOURCE_DATA_ENTRY*)((DWORD)pFileBuffer + RVATOFOA(pFileBuffer, DATA_DIR->VirtualAddress));
			wprintf(L"      -->资源RVA:%08X     尺寸:%X\n", DATA_DIR->VirtualAddress, DATA_DIR->Size);
			return;
		}
	}

	return;
}


void ShowAllProc(HWND hWndProc) {
	ListView_DeleteAllItems(hWndProc);
	LVITEMA item = {};
	item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
	item.state = 0;
	item.stateMask = 0;
	TCHAR str[260] = {};
	int cnt=0;
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 process = { sizeof(PROCESSENTRY32) };
	while (Process32Next(hProcessSnap, &process)) {

		item.iItem = cnt;
		item.iImage = cnt;
		


		HANDLE hle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process.th32ProcessID);
		TCHAR str[260] = {};
		item.iSubItem = 0;
		if (hle) {
			if (GetModuleFileNameEx(hle, 0, str, 260)) {
				item.pszText = (char*)str;
			}
			else {
				item.pszText = (char*)process.szExeFile;
			}
			CloseHandle(hle);
		}
		else {
			item.pszText = (char*)process.szExeFile;
		}
		ListView_InsertItem(hWndProc, &item);


		item.iSubItem = 1;
		wsprintf(str, TEXT("%d"), process.th32ProcessID);
		item.pszText = (char*)str;
		ListView_SetItem(hWndProc, &item);

		HANDLE hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, process.th32ProcessID);
		if (hModuleSnap) {
			MODULEENTRY32 module = { sizeof(MODULEENTRY32) };
			if (Module32Next(hModuleSnap, &module)) {

				item.iSubItem = 2;
				wsprintf(str, TEXT("0x%08X"), module.modBaseAddr);
				item.pszText = (char*)str;
				ListView_SetItem(hWndProc, &item);
				item.iSubItem = 3;
				wsprintf(str, TEXT("0x%08X"), module.modBaseSize);
				item.pszText = (char*)str;
				ListView_SetItem(hWndProc, &item);
				
			}
			else {
				item.iSubItem = 2;
				wsprintf(str, TEXT("0x%08X"), 0);
				item.pszText = (char*)str;
				ListView_SetItem(hWndProc, &item);
				item.iSubItem = 3;
				wsprintf(str, TEXT("0x%08X"), 0);
				item.pszText = (char*)str;
				ListView_SetItem(hWndProc, &item);

				DbgwPrintf(L"获取模块基址失败!错误代码为:%d\n", GetLastError());
			}
			CloseHandle(hModuleSnap);
		}
		else {
			item.iSubItem = 2;
			wsprintf(str, TEXT("0x%08X"), 0);
			item.pszText = (char*)str;
			ListView_SetItem(hWndProc, &item);
			item.iSubItem = 3;
			wsprintf(str, TEXT("0x%08X"), 0);
			item.pszText = (char*)str;
			ListView_SetItem(hWndProc, &item);
		}
		cnt++;
	}
}

void ShowAllModules(HWND hWndModule,DWORD PID) {
	ListView_DeleteAllItems(hWndModule);
	LVITEMA item = {};
	item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
	item.state = 0;
	item.stateMask = 0;
	int cnt=0;
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, PID);
	if ((DWORD)hProcessSnap == -1) {
		DbgwPrintf(L"%d\n", GetLastError());
	}
	MODULEENTRY32 module = { sizeof(MODULEENTRY32) };
	
	while (Module32Next(hProcessSnap,&module)) {
		item.iItem = cnt;
		item.iImage = cnt;
		TCHAR szModName[MAX_PATH] = {};
		// Get the full path to the module's file.
		item.iSubItem = 0;
		item.pszText = (char*)module.szExePath;
		ListView_InsertItem(hWndModule, &item);
		item.iSubItem = 1;
		wsprintf(szModName, TEXT("0x%08X"), module.modBaseAddr);
		item.pszText = (char*)szModName;
		ListView_SetItem(hWndModule, &item);
		item.iSubItem = 2;
		wsprintf(szModName, TEXT("0x%08X"), module.modBaseSize);
		item.pszText = (char*)szModName;
		ListView_SetItem(hWndModule, &item);
		
		cnt++;
	}
	if(hProcessSnap)
		CloseHandle(hProcessSnap);
	
}


void OpenFileButton() {
	OPENFILENAME file = { sizeof(OPENFILENAME) };
	
	file.lStructSize = sizeof(OPENFILENAME);
	file.hwndOwner = NULL;
	file.lpstrFile = szFile;
	file.lpstrFile[0] = '\0';
	file.nMaxFile = sizeof(szFile);
	file.lpstrFilter = TEXT("*.exe;*.dll;*.sys;\0*.exe;*.dll;*.sys;\0");
	file.nFilterIndex = 1;
	file.lpstrFileTitle = NULL;
	file.nMaxFileTitle = 0;
	file.lpstrInitialDir = NULL;
	if (GetOpenFileName(&file)) {
		DbgwPrintf(L"选中%s\n", szFile);
	}
	else {
		DbgwPrintf(L"打开文件失败,错误代码为:%d\n", GetLastError());
	}
}


BOOL CALLBACK Dlgproc_Main(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam) {
	switch (Msg) {
	case WM_INITDIALOG: {
		InitListControlMain(hWnd);
		ShowAllProc(PROCHWND);
		break;
	}
	case WM_NOTIFY: {
		switch (wParam) {
		case IDC_LIST_PROC: {
			TCHAR str[20] = {};
			NMLISTVIEW* listview = (NMLISTVIEW*)lParam;
			switch (listview->hdr.code) {
			case NM_CLICK: {
				ListView_GetItemText(PROCHWND, listview->iItem, 1, str, 20);
				ShowAllModules(MODULEHWND, _wtoi(str));
				break;
			}
			}

			break;
		}
		}
		break;
	}
	case WM_COMMAND: {
		switch (LOWORD(wParam)) {
		case WM_DESTROY: {
			PostQuitMessage(0);
			break;
		}
		case IDC_BUTTON_QUIT: {
			MessageBox(hWnd, TEXT("感谢使用!"), TEXT(""), 0);
			PostQuitMessage(0);
			break;
		}
		case IDC_BUTTON_ABOUT: {
			DialogBox(hAppInstance, MAKEINTRESOURCE(IDD_DIALOG_ABOUT), hWnd, Dlgproc_About);
			break;
		}
		case IDC_BUTTON_OPEN:
			DbgwPrintf(L"部分进程加载:\n");
			OpenFileButton();
			DialogBox(hAppInstance, MAKEINTRESOURCE(IDD_DIALOG_PE), hWnd, Dlgproc_PE);
			break;
		}
	}
	default: {
		return FALSE;
	}
	}
	return TRUE;
}
void InitListControlMain(HWND hWnd) {
	HWND hListCtrl = GetDlgItem(hWnd, IDC_LIST_PROC);
	SendMessage(hListCtrl, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
	PROCHWND = hListCtrl;
	LVCOLUMN lvCol = { 0 };
	lvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	lvCol.fmt = LVCFMT_LEFT;
	lvCol.iSubItem = 0;
	lvCol.cx = 300;
	lvCol.pszText = (LPTSTR)TEXT("进程名称");
	ListView_InsertColumn(hListCtrl, 0, &lvCol);

	lvCol.iSubItem = 1;
	lvCol.cx = 100;
	lvCol.pszText = (LPTSTR)TEXT("PID");
	ListView_InsertColumn(hListCtrl, 1, &lvCol);

	lvCol.iSubItem = 2;
	lvCol.cx = 150;
	lvCol.pszText = (LPTSTR)TEXT("映像基址");
	ListView_InsertColumn(hListCtrl, 2, &lvCol);

	lvCol.iSubItem = 3;
	lvCol.cx = 150;
	lvCol.pszText = (LPTSTR)TEXT("映像大小");
	ListView_InsertColumn(hListCtrl, 3, &lvCol);



	hListCtrl = GetDlgItem(hWnd, IDC_LIST_MODULE);
	SendMessage(hListCtrl, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
	MODULEHWND = hListCtrl;
	lvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	lvCol.fmt = LVCFMT_LEFT;
	lvCol.iSubItem = 0;
	lvCol.cx = 300;
	lvCol.pszText = (LPTSTR)TEXT("模块名称");
	ListView_InsertColumn(hListCtrl, 0, &lvCol);

	lvCol.iSubItem = 1;
	lvCol.cx = 150;
	lvCol.pszText = (LPTSTR)TEXT("模块映像基址");
	ListView_InsertColumn(hListCtrl, 1, &lvCol);

	lvCol.iSubItem = 2;
	lvCol.cx = 150;
	lvCol.pszText = (LPTSTR)TEXT("模块映像大小");
	ListView_InsertColumn(hListCtrl, 2, &lvCol);
}
BOOL CALLBACK Dlgproc_PE(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	
	switch (Msg) {
	case WM_INITDIALOG: {
		ShowDosHeader(hWnd);
		break;
	}
 
	case WM_COMMAND: {
		switch (LOWORD(wParam)) {
		case WM_DESTROY: {
			//DestroyWindow(hWnd);
			EndDialog(hWnd, IDCANCEL);
			DbgwPrintf(L"部分进程结束:\n");
			break;
		}
		case BUTTON_FILE_HEADER: {
			DialogBox(hAppInstance, MAKEINTRESOURCE(IDD_DIALOG_FILEHEADER), hWnd, Dlgproc_FileHeader);
			
			break;
		}
		case BUTTON_OPTIONAL_HEADER: {
			DialogBox(hAppInstance, MAKEINTRESOURCE(IDD_DIALOG_OPTIONALHEADER), hWnd, Dlgproc_OptionalHeader);
			break;
		}
		case BUTTON_SECTION: {
			DialogBox(hAppInstance, MAKEINTRESOURCE(IDD_DIALOG_SECTIONS), hWnd, Dlgproc_Sections);
			break;
		}
		case BUTTON_DATADIR: {
			DialogBox(hAppInstance, MAKEINTRESOURCE(IDD_DIALOG_DATADIR), hWnd, Dlgproc_DataDirectory);
			break;
		}
		}
		break;
	}
	default: {
		return FALSE;
	}
	}
	return TRUE;
}
void ShowDosHeader(HWND hWndPE) {
	CHAR Path[MAX_PATH] = {};
	TCHAR str[20] = {};
	sprintf_s(Path, MAX_PATH,"%ws", szFile);
	FileToFileBuffer(Path, &FileBuffer);
	
	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)FileBuffer;
	
	wsprintf(str, L"%04X", DOS_HEADER->e_magic);
	SetDlgItemText(hWndPE, EDIT_MAGICNUM, str);

	wsprintf(str, L"%04X", DOS_HEADER->e_cblp);
	SetDlgItemText(hWndPE, EDIT_BYTE, str);

	wsprintf(str, L"%04X", DOS_HEADER->e_cp);
	SetDlgItemText(hWndPE, EDIT_PAGES, str);

	wsprintf(str, L"%04X", DOS_HEADER->e_crlc);
	SetDlgItemText(hWndPE, EDIT_RELOC, str);

	wsprintf(str, L"%04X", DOS_HEADER->e_cparhdr);
	SetDlgItemText(hWndPE, EDIT_SIZE, str);

	wsprintf(str, L"%04X", DOS_HEADER->e_minalloc);
	SetDlgItemText(hWndPE, EDIT_MIN, str);

	wsprintf(str, L"%04X", DOS_HEADER->e_maxalloc);
	SetDlgItemText(hWndPE, EDIT_MAX, str);

	wsprintf(str, L"%04X", DOS_HEADER->e_ss);
	SetDlgItemText(hWndPE, EDIT_SS, str);

	wsprintf(str, L"%04X", DOS_HEADER->e_sp);
	SetDlgItemText(hWndPE, EDIT_SP, str);

	wsprintf(str, L"%04X", DOS_HEADER->e_csum);
	SetDlgItemText(hWndPE, EDIT_CHECKSUM, str);

	wsprintf(str, L"%04X", DOS_HEADER->e_ip);
	SetDlgItemText(hWndPE, EDIT_IP, str);

	wsprintf(str, L"%04X", DOS_HEADER->e_cs);
	SetDlgItemText(hWndPE, EDIT_CS, str);

	wsprintf(str, L"%04X", DOS_HEADER->e_lfarlc);
	SetDlgItemText(hWndPE, EDIT_TO, str);

	wsprintf(str, L"%04X", DOS_HEADER->e_ovno);
	SetDlgItemText(hWndPE, EDIT_ON, str);

	wsprintf(str, L"%04X", DOS_HEADER->e_oemid);
	SetDlgItemText(hWndPE, EDIT_OI, str);

	wsprintf(str, L"%04X", DOS_HEADER->e_oeminfo);
	SetDlgItemText(hWndPE, EDIT_OI2, str);

	wsprintf(str, L"%08X", DOS_HEADER->e_lfanew);
	SetDlgItemText(hWndPE, EDIT_PA, str);

	return;
}

BOOL CALLBACK Dlgproc_About(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	switch (Msg) {
	case WM_COMMAND: {
		switch (LOWORD(wParam)) {
		case WM_DESTROY: {
			EndDialog(hWnd, IDCANCEL);
			break;
		}
		}
		break;
	}
	default: {
		return FALSE;
	}
	}
	return TRUE;
}
BOOL CALLBACK Dlgproc_FileHeader(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	switch (Msg) {
	case WM_INITDIALOG: {
		ShowFileHeader(hWnd);
		break;
	}
	case WM_COMMAND: {
		switch (LOWORD(wParam)) {
		case WM_DESTROY: {
			EndDialog(hWnd, IDCANCEL);
			break;
		}
		}
		break;
	}
	default: {
		return FALSE;
	}
	}
	return TRUE;
}
BOOL CALLBACK Dlgproc_OptionalHeader(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	switch (Msg) {
	case WM_INITDIALOG: {
		ShowOptionalHeader(hWnd);
		break;
	}
	case WM_COMMAND: {
		switch (LOWORD(wParam)) {
		case WM_DESTROY: {
			EndDialog(hWnd, IDCANCEL);
			break;
		}
		}
		break;
	}
	default: {
		return FALSE;
	}
	}
	return TRUE;
}
BOOL CALLBACK Dlgproc_Sections(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	switch (Msg) {
	case WM_INITDIALOG: {
		InitListControlSection(hWnd);
		ShowAllSection(SECTIONHWND);
		break;
	}
	case WM_COMMAND: {
		switch (LOWORD(wParam)) {
		case WM_DESTROY: {
			EndDialog(hWnd, IDCANCEL);
			break;
		}
		}
		break;
	}
	default: {
		return FALSE;
	}
	}
	return TRUE;
}
BOOL CALLBACK Dlgproc_DataDirectory(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {

	switch (Msg) {
	case WM_INITDIALOG: {
		ShowDataDirectory(hWnd);
		break;
	}

	case WM_COMMAND: {
		switch (LOWORD(wParam)) {
		case WM_DESTROY: {
			//DestroyWindow(hWnd);
			EndDialog(hWnd, IDCANCEL);
			DbgwPrintf(L"部分进程结束:\n");
			break;
		}
		}
		break;
	}
	default: {
		return FALSE;
	}
	}
	return TRUE;
}
void InitListControlSection(HWND hWnd) {
	HWND hListCtrl = GetDlgItem(hWnd, IDC_LIST_SECTIONS);
	SECTIONHWND = hListCtrl;
	SendMessage(hListCtrl, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
	PROCHWND = hListCtrl;
	LVCOLUMN lvCol = { 0 };
	lvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	lvCol.fmt = LVCFMT_LEFT;
	lvCol.iSubItem = 0;
	lvCol.cx = 50;
	lvCol.pszText = (LPTSTR)TEXT("编号");
	ListView_InsertColumn(hListCtrl, 0, &lvCol);

	lvCol.iSubItem = 1;
	lvCol.cx = 100;
	lvCol.pszText = (LPTSTR)TEXT("段名称");
	ListView_InsertColumn(hListCtrl, 1, &lvCol);

	lvCol.iSubItem = 2;
	lvCol.cx = 100;
	lvCol.pszText = (LPTSTR)TEXT("内存中尺寸");
	ListView_InsertColumn(hListCtrl, 2, &lvCol);

	lvCol.iSubItem = 3;
	lvCol.cx = 100;
	lvCol.pszText = (LPTSTR)TEXT("内存中偏移");
	ListView_InsertColumn(hListCtrl, 3, &lvCol);

	lvCol.iSubItem = 4;
	lvCol.cx = 100;
	lvCol.pszText = (LPTSTR)TEXT("文件中尺寸");
	ListView_InsertColumn(hListCtrl, 4, &lvCol);

	lvCol.iSubItem = 5;
	lvCol.cx = 100;
	lvCol.pszText = (LPTSTR)TEXT("文件中偏移");
	ListView_InsertColumn(hListCtrl, 5, &lvCol);


	lvCol.iSubItem = 6;
	lvCol.cx = 100;
	lvCol.pszText = (LPTSTR)TEXT("属性");
	ListView_InsertColumn(hListCtrl, 6, &lvCol);

	return;
}
void ShowAllSection(HWND hWndSection) {
	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)FileBuffer;
	_IMAGE_NT_HEADERS* NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)FileBuffer + DOS_HEADER->e_lfanew);
	_IMAGE_SECTION_HEADER* SECTION_HEADER = (_IMAGE_SECTION_HEADER*)((DWORD)FileBuffer + DOS_HEADER->e_lfanew + 0x4 + NT_HEADERS->FileHeader.SizeOfOptionalHeader + sizeof(_IMAGE_FILE_HEADER));
	LVITEMA item = {};
	item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
	item.state = 0;
	item.stateMask = 0;
	for (int i = 0; i < NT_HEADERS->FileHeader.NumberOfSections; i++) {
		item.iItem = i;
		item.iImage = i;
		TCHAR szModName[MAX_PATH] = {};
		// Get the full path to the module's file.
		item.iSubItem = 0;
		wsprintf(szModName, L"%d", i + 1);
		item.pszText = (char*)szModName;
		ListView_InsertItem(hWndSection, &item);

		item.iSubItem = 1;
		MultiByteToWideChar(CP_ACP, MB_COMPOSITE, (char*)(SECTION_HEADER + i)->Name, 8, szModName, MAX_PATH);
		item.pszText = (char*)szModName;
		ListView_SetItem(hWndSection, &item);

		item.iSubItem = 2;
		wsprintf(szModName, L"%08X", (SECTION_HEADER + i)->Misc.VirtualSize);
		item.pszText = (char*)szModName;
		ListView_SetItem(hWndSection, &item);

		item.iSubItem = 3;
		wsprintf(szModName, L"%08X", (SECTION_HEADER + i)->VirtualAddress);
		item.pszText = (char*)szModName;
		ListView_SetItem(hWndSection, &item);

		item.iSubItem = 4;
		wsprintf(szModName, L"%08X", (SECTION_HEADER + i)->SizeOfRawData);
		item.pszText = (char*)szModName;
		ListView_SetItem(hWndSection, &item);

		item.iSubItem = 5;
		wsprintf(szModName, L"%08X", (SECTION_HEADER + i)->PointerToRawData);
		item.pszText = (char*)szModName;
		ListView_SetItem(hWndSection, &item);

		item.iSubItem = 6;
		wsprintf(szModName, L"%08X", (SECTION_HEADER + i)->Characteristics);
		item.pszText = (char*)szModName;
		ListView_SetItem(hWndSection, &item);
	}
}
void ShowFileHeader(HWND hWndFH) {
	TCHAR str[20] = {};
	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)FileBuffer;
	_IMAGE_NT_HEADERS* NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)FileBuffer + DOS_HEADER->e_lfanew);

	wsprintf(str, L"%04X", NT_HEADERS->FileHeader.Machine);
	SetDlgItemText(hWndFH, EDIT_MACHINE, str);

	wsprintf(str, L"%04X", NT_HEADERS->FileHeader.NumberOfSections);
	SetDlgItemText(hWndFH, EDIT_SECTIONNUM, str);

	wsprintf(str, L"%08X", NT_HEADERS->FileHeader.TimeDateStamp);
	SetDlgItemText(hWndFH, EDIT_TIMEDATE_FILEHEADER, str);

	wsprintf(str, L"%08X", NT_HEADERS->FileHeader.PointerToSymbolTable);
	SetDlgItemText(hWndFH, EDIT_SYMBOLPOINT, str);

	wsprintf(str, L"%08X", NT_HEADERS->FileHeader.NumberOfSymbols);
	SetDlgItemText(hWndFH, EDIT_SYMBOLNUM, str);

	wsprintf(str, L"%04X", NT_HEADERS->FileHeader.SizeOfOptionalHeader);
	SetDlgItemText(hWndFH, EDIT_OPTIONALHEADERSIZE, str);

	wsprintf(str, L"%04X", NT_HEADERS->FileHeader.Characteristics);
	SetDlgItemText(hWndFH, EDIT_CHARACTER_FILEHEADER, str);
	return;
}
void ShowOptionalHeader(HWND hWndOH) {
	TCHAR str[20] = {};
	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)FileBuffer;
	_IMAGE_NT_HEADERS* NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)FileBuffer + DOS_HEADER->e_lfanew);

	wsprintf(str, L"%04X", NT_HEADERS->OptionalHeader.Magic);
	SetDlgItemText(hWndOH, EDIT_MAGICNUM_OPTIONALHEADER, str);

	wsprintf(str, L"%02X", NT_HEADERS->OptionalHeader.MajorLinkerVersion);
	SetDlgItemText(hWndOH, EDIT_MAJORLINKERVERSION, str);

	wsprintf(str, L"%02X", NT_HEADERS->OptionalHeader.MinorLinkerVersion);
	SetDlgItemText(hWndOH, EDIT_MINORLINKERVERSION, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.SizeOfCode);
	SetDlgItemText(hWndOH, EDIT_SIZEOFCODE, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.SizeOfInitializedData);
	SetDlgItemText(hWndOH, EDIT_SIZEOFINITDATA, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.SizeOfUninitializedData);
	SetDlgItemText(hWndOH, EDIT_SIZEOFUNINITDATA, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.AddressOfEntryPoint);
	SetDlgItemText(hWndOH, EDIT_OEP, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.BaseOfCode);
	SetDlgItemText(hWndOH, EDIT_CODEBASE, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.BaseOfData);
	SetDlgItemText(hWndOH, EDIT_DATABASE, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.ImageBase);
	SetDlgItemText(hWndOH, EDIT_IMAGEBASE, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.SectionAlignment);
	SetDlgItemText(hWndOH, EDIT_SECTIONALIGNMENT, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.FileAlignment);
	SetDlgItemText(hWndOH, EDIT_FILEALIGNMENT, str);

	wsprintf(str, L"%04X", NT_HEADERS->OptionalHeader.MajorOperatingSystemVersion);
	SetDlgItemText(hWndOH, EDIT_MAJOROSVERSION, str);

	wsprintf(str, L"%04X", NT_HEADERS->OptionalHeader.MinorOperatingSystemVersion);
	SetDlgItemText(hWndOH, EDIT_MINOROSVERSION, str);

	wsprintf(str, L"%04X", NT_HEADERS->OptionalHeader.MajorImageVersion);
	SetDlgItemText(hWndOH, EDIT_MAJORIMAGEVERSION, str);

	wsprintf(str, L"%04X", NT_HEADERS->OptionalHeader.MinorImageVersion);
	SetDlgItemText(hWndOH, EDIT_MINORIMAGEVERSION, str);

	wsprintf(str, L"%04X", NT_HEADERS->OptionalHeader.MajorSubsystemVersion);
	SetDlgItemText(hWndOH, EDIT_MAJORSSVERSION, str);

	wsprintf(str, L"%04X", NT_HEADERS->OptionalHeader.MinorSubsystemVersion);
	SetDlgItemText(hWndOH, EDIT_MINORSSVERSION, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.Win32VersionValue);
	SetDlgItemText(hWndOH, EDIT_WIN32VERSION, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.SizeOfImage);
	SetDlgItemText(hWndOH, EDIT_SIZEIMAGE, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.SizeOfHeaders);
	SetDlgItemText(hWndOH, EDIT_SIZEHEADER, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.CheckSum);
	SetDlgItemText(hWndOH, EDIT_CHECKSUM_OPTIONALHEADER, str);

	wsprintf(str, L"%04X", NT_HEADERS->OptionalHeader.Subsystem);
	SetDlgItemText(hWndOH, EDIT_SUBSYSTEM, str);

	wsprintf(str, L"%04X", NT_HEADERS->OptionalHeader.DllCharacteristics);
	SetDlgItemText(hWndOH, EDIT_DLLCHARACTER, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.SizeOfStackReserve);
	SetDlgItemText(hWndOH, EDIT_STACKR, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.SizeOfStackCommit);
	SetDlgItemText(hWndOH, EDIT_STACKC, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.SizeOfHeapReserve);
	SetDlgItemText(hWndOH, EDIT_HEAPR, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.SizeOfHeapCommit);
	SetDlgItemText(hWndOH, EDIT_HEAPC, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.LoaderFlags);
	SetDlgItemText(hWndOH, EDIT_LOADERFLAGS, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.NumberOfRvaAndSizes);
	SetDlgItemText(hWndOH, EDIT_RVANUMBER, str);
}
void ShowDataDirectory(HWND hWndDD) {
	TCHAR str[20] = {};
	_IMAGE_DOS_HEADER* DOS_HEADER = (_IMAGE_DOS_HEADER*)FileBuffer;
	_IMAGE_NT_HEADERS* NT_HEADERS = (_IMAGE_NT_HEADERS*)((DWORD)FileBuffer + DOS_HEADER->e_lfanew);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[0].VirtualAddress);
	SetDlgItemText(hWndDD, EDIT_EXPORTTABLE_RVA, str);
	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[0].Size);
	SetDlgItemText(hWndDD, EDIT_EXPORTTABLE_SIZE, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[1].VirtualAddress);
	SetDlgItemText(hWndDD, EDIT_IMPORTTABLE_RVA, str);
	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[1].Size);
	SetDlgItemText(hWndDD, EDIT_IMPORTTABLE_SIZE, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[2].VirtualAddress);
	SetDlgItemText(hWndDD, EDIT_RESOURCETABLE_RVA, str);
	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[2].Size);
	SetDlgItemText(hWndDD, EDIT_RESOURCETABLE_SIZE, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[3].VirtualAddress);
	SetDlgItemText(hWndDD, EDIT_EXTABLE_RVA, str);
	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[3].Size);
	SetDlgItemText(hWndDD, EDIT_EXTABLE_SIZE, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[4].VirtualAddress);
	SetDlgItemText(hWndDD, EDIT_SETABLE_RVA, str);
	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[4].Size);
	SetDlgItemText(hWndDD, EDIT_SETABLE_SIZE, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[5].VirtualAddress);
	SetDlgItemText(hWndDD, EDIT_BRTABLE_RVA, str);
	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[5].Size);
	SetDlgItemText(hWndDD, EDIT_BRTABLE_SIZE, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[6].VirtualAddress);
	SetDlgItemText(hWndDD, EDIT_DEBUGTABLE_RVA, str);
	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[6].Size);
	SetDlgItemText(hWndDD, EDIT_DEBUGTABLE_SIZE, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[7].VirtualAddress);
	SetDlgItemText(hWndDD, EDIT_ASTABLE_RVA, str);
	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[7].Size);
	SetDlgItemText(hWndDD, EDIT_ASTABLE_SIZE, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[8].VirtualAddress);
	SetDlgItemText(hWndDD, EDIT_GPRVA_RVA, str);
	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[8].Size);
	SetDlgItemText(hWndDD, EDIT_GPRVA_SIZE, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[9].VirtualAddress);
	SetDlgItemText(hWndDD, EDIT_TLSTABLE_RVA, str);
	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[9].Size);
	SetDlgItemText(hWndDD, EDIT_TLSTABLE_SIZE, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[10].VirtualAddress);
	SetDlgItemText(hWndDD, EDIT_LCTABLE_RVA, str);
	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[10].Size);
	SetDlgItemText(hWndDD, EDIT_LCTABLE_SIZE, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[11].VirtualAddress);
	SetDlgItemText(hWndDD, EDIT_BOUNDIMPORTTABLE_RVA, str);
	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[11].Size);
	SetDlgItemText(hWndDD, EDIT_BOUNDIMPORTTABLE_SIZE, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[12].VirtualAddress);
	SetDlgItemText(hWndDD, EDIT_IATTABLE_RVA, str);
	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[12].Size);
	SetDlgItemText(hWndDD, EDIT_IATTABLE_SIZE, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[13].VirtualAddress);
	SetDlgItemText(hWndDD, EDIT_DELAYIMPORTTABLE_RVA, str);
	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[13].Size);
	SetDlgItemText(hWndDD, EDIT_DELAYIMPORTTABLE_SIZE, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[14].VirtualAddress);
	SetDlgItemText(hWndDD, EDIT_COMTABLE_RVA, str);
	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[14].Size);
	SetDlgItemText(hWndDD, EDIT_COMTABLE_SIZE, str);

	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[14].VirtualAddress);
	SetDlgItemText(hWndDD, EDIT_RESERVED_RVA, str);
	wsprintf(str, L"%08X", NT_HEADERS->OptionalHeader.DataDirectory[14].Size);
	SetDlgItemText(hWndDD, EDIT_RESERVED_SIZE, str);
}