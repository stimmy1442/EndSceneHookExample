#include <windows.h>
#include <tchar.h>
#include <malloc.h>
#include <iostream>

using namespace std;


int main(void) {
  /* WORKING PRINCIPLE
  This is a pretty straightforward DLL injection tool.
  One should know that a DLL acts like it's part of the program it is loaded into. It can access the program's memory, can contain functions which are called by the program, and so on.
  By DLL injection, the program is forced to load an additional DLL which it normally doesn't load. This offers the opportunity to modify the program's behavior in various ways.
  DLL injection is done in the following steps:
  - Start the program (Halo) via CreateProcess as a suspended task. This causes the program to be loaded into memory, but code execution will not start yet.
  - Reserve memory in Halo's address space using the VirtualAllocEx function
  - Write the path/name of the DLL to be injected into the reserved memory
  - Get the address of the LoadLibrary function. 
  - Call CreateRemoteThread in Halo's process with the address of LoadLibrary, and the DLL file name as a parameter. This will load the DLL into Halo, and execute its DLLMain. Now, the trainer is running.
  - Wait for the DllMain to exit via WaitForSingleObject and check its return value. The injected DLL is designed in a way that the DllMain exits after creating a thread. It will return TRUE if the thread was successfully started, and FALSE if it failed.
  - Resume the Halo process, remember that is was started suspended. Now, Halo will start. This tool will exit then, the trainer will run inside Halo until Halo is closed.

  Also, via CreateFileMapping a shared memory section is created. The XML configuration is transferred from this DLL injector to the trainer via this route.
 */
 
  // Get DLL path = Exe folder + DLL name
  char ErrStr[256];
  wchar_t szDLLFileName[MAX_PATH];
  GetModuleFileNameW(NULL, szDLLFileName, MAX_PATH);
  wchar_t *cx = wcsrchr(szDLLFileName, '\\');
  if (cx) cx[0] = 0;
  wchar_t* DLLName = wcscat(szDLLFileName, L"\\InjectedDLL.dll");
	const char* HaloFile = "D:\\Spiele\\Halo_CE\\SPV3\\haloce.exe";
  const char* HaloFolder = "D:\\Spiele\\Halo_CE\\SPV3";
  // The DLL injection part

	STARTUPINFOA StartInfo;
	ZeroMemory(&StartInfo, sizeof(STARTUPINFOA));
	StartInfo.cb = sizeof(STARTUPINFOA);
	PROCESS_INFORMATION ProcInfo;
	ZeroMemory(&ProcInfo, sizeof(PROCESS_INFORMATION));

	if (CreateProcessA(HaloFile, "-vidmode 1920,1080,60 -console -devmode", NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, HaloFolder, &StartInfo, &ProcInfo) == FALSE) {
		strcpy(ErrStr, "Could not start Halo. Maybe the EXE name is invalid");
		goto err3;
	}

	LPVOID RemoteString = (LPVOID)VirtualAllocEx(ProcInfo.hProcess, NULL, wcslen(DLLName) * 2 + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (RemoteString == NULL) {
		strcpy(ErrStr, "Couldn't allocate memory for DLL name in remote process");
		goto err2;
	}

	if (WriteProcessMemory(ProcInfo.hProcess, RemoteString, DLLName, wcslen(DLLName) * 2 + 1, NULL) == FALSE) {
		strcpy(ErrStr, "Error writing DLL name to target process");
		goto err1;
	}

	PTHREAD_START_ROUTINE LibAddr = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryW");
	HANDLE hRThread = CreateRemoteThread(ProcInfo.hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)LibAddr, RemoteString, NULL, NULL);
	if (hRThread == NULL) {
		strcpy(ErrStr, "Error starting the DLL in remote process");
		goto err1;
	}
	WaitForSingleObject(hRThread, INFINITE);
	VirtualFreeEx(ProcInfo.hProcess, RemoteString, 0, MEM_RELEASE);
	HINSTANCE hHookDll = NULL;
	GetExitCodeThread(hRThread, (LPDWORD)&hHookDll);
	if (hHookDll == NULL) {
		strcpy(ErrStr, "Injected DLL has crashed!");
		goto err1;
	}

	CloseHandle(hRThread);
	ResumeThread(ProcInfo.hThread);
	CloseHandle(ProcInfo.hThread);
	CloseHandle(ProcInfo.hProcess);
	return 0;

err1:
	VirtualFreeEx(ProcInfo.hProcess, RemoteString, 0, MEM_RELEASE);
err2:
	CloseHandle(ProcInfo.hThread);
	TerminateProcess(ProcInfo.hProcess, 0);
	CloseHandle(ProcInfo.hProcess);
err3:
	MessageBoxA(0, ErrStr, "DLL Injector", MB_OK | MB_ICONHAND);
	return 0;
}