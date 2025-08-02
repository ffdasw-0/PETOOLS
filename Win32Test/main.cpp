#include"stdafx.h"

HINSTANCE hAppInstance;

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nShowCmd) {
	/*TCHAR* ClassName=(TCHAR*)TEXT("My Windows");
	WNDCLASS WndClass = {};
	WndClass.hInstance = hInstance;
	WndClass.lpszClassName = ClassName;
	WndClass.lpfnWndProc = WndProc;
	WndClass.hbrBackground = (HBRUSH)COLOR_MENU;
	
	RegisterClass(&WndClass);*/
	//GetAllProcCurrent();
	hAppInstance = hInstance;
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG_MAIN), NULL, Dlgproc_Main);

	
	return 0;

}