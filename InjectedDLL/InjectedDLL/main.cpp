/*
Direct3D EndScene Hooking Example
This is an example for hooking the Direct3D EndScene() function.
EndScene is called by the game when all graphics elements are drawn, and the game intends to render + display the frame via Direct3D.
By hooking the function, one can draw additional elements on the screen before calling the actual EndScene function.
This is useful for displaying FPS counters, and of course for game hacking/cheating.
In this small example, just the text "Hello World" is displayed.

Hooking the function works in several steps:
EndScene is a virtual function, calles via a function pointer in memory. The function pointer is found via a magic number and an offset from it; this code is not mine.
Source of the hooking code: https://www.unknowncheats.me/wiki/Direct3D:DirectX_9_EndScene_Midfunction_Hook_Example
The original code uses the pretty old Microsoft Detours.
Since there already is a function pointer (= entry address of a function) in the VMT, one can just replace this pointer with another one to hook the function.
Using an API hooking framework like Detours or inserting a JMP instruction somewhere is completely unnecessary.

Therefore, the procedure is:
- Find the VMT in the memory, using the FindPattern function
- Back up the original function pointer of EndScene
- Replace it with the address of a custom function, which lies within this DLL
- Done!
Now let's talk about this custom function. It must accomplish several tasks:
- Fetch the pointer to the Direct3D Device (LPDIRECT3DDEVICE9) used for drawing from the stack. This is needed for drawing further graphics elements
- Call another custom function (BeforeEndScene()), which can draw additional elements. This function gets the LPDIRECT3DDEVICE9 as a parameter
- Clean up the stack and registers. Everything must be in the same state as it would be if the hook wasn't there
- Jump to the original Direct3D EndScene() entry point, which was backed up while hooking the VMT. This function now works as usual and renders the frame.

The custom BeforeEndScene() function may draw additonal graphics elements using the standard D3D drawing functions, which are also rendered like they were part of the game. 
This is the main reason why the EndScene hooking is so popular for game overlays.

Happy testing/coding/cheating,

Stimmy
*/


#define _CRT_SECURE_NO_WARNINGS


#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>
#include "stdint.h"


#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

volatile DWORD dwEndscene_hook;
volatile DWORD dwEndscene_ret;

HINSTANCE hDllModule;

DWORD *vTable;
volatile DWORD dmghook_ret;

LPD3DXFONT pFont;

static volatile LPDIRECT3DDEVICE9 pDevice;

VOID WriteText(LPDIRECT3DDEVICE9 pDevice, INT x, INT y, DWORD color, CHAR *text)
{
  RECT rect;
  SetRect(&rect, x, y, x, y);
  pFont->DrawText(NULL, text, -1, &rect, DT_NOCLIP | DT_LEFT, color);
}

VOID WINAPI BeforeEndScene(LPDIRECT3DDEVICE9 pDevice)
{
  //The custom function which is called before calling EndScene() 
  D3DXCreateFont(pDevice, 30, 0, FW_BOLD, 1, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial", &pFont);
  WriteText(pDevice, 0, 0, D3DCOLOR_ARGB(255, 255, 0, 0), "Hello World!");  //Display Hello World
  pFont->Release();
}

DWORD HookVMT(void *address, void *newfunction)
{
  DWORD NewProtection;
  DWORD retval;
  VirtualProtect(address, 4, PAGE_EXECUTE_READWRITE, &NewProtection);
  memcpy(&retval, address, 4);
  memcpy(address, newfunction, 4);
  VirtualProtect(address, 4, NewProtection, &NewProtection);
  return retval;
}

// The function which the VMT hook jumps to
__declspec(naked) void hook()
{
  static volatile LPDIRECT3DDEVICE9 pDevice;
  // get arguments from registers
  __asm {
    // Push registers
    pushad
    pushfd
    push esi
    // Get LPDIRECT3DDEVICE9 for drawing from stack
    mov esi, dword ptr ss : [esp + 0x2c];
    mov pDevice, esi;
  }

  {
    BeforeEndScene(pDevice);
  }
  __asm {
    // Clean up stack/registers. Everything must look like the function was never hooked
    pop esi
    popfd
    popad
    jmp[dwEndscene_ret] // jump to original EndScene
  }
}

bool Mask(const BYTE* pData, const BYTE* bMask, const char* szMask)
{
  for (; *szMask; ++szMask, ++pData, ++bMask)
    if (*szMask == 'x' && *pData != *bMask)
      return false;
  return (*szMask) == NULL;
}

DWORD FindPattern(DWORD dwAddress, DWORD dwLen, BYTE *bMask, char * szMask)
{
  for (DWORD i = 0; i<dwLen; i++)
    if (Mask((BYTE*)(dwAddress + i), bMask, szMask))
      return (DWORD)(dwAddress + i);
  return 0;
}

void MyHook(void)
{
  DWORD hD3D = NULL;
  //Find VMT in memory
  while (!hD3D) hD3D = (DWORD)GetModuleHandle("d3d9.dll");
  DWORD PPPDevice = FindPattern(hD3D, 0x128000, (PBYTE)"\xC7\x06\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\x89\x86", "xx????xx????xx");
  //Copy VMT address
  memcpy(&vTable, (void *)(PPPDevice + 2), 4);

  DWORD NewProtection;
  // Hook the EndScene function in VMT; straightforward by replacing the function pointer
  VirtualProtect(&vTable[42], 4, PAGE_EXECUTE_READWRITE, &NewProtection);
  dwEndscene_ret = vTable[42];  //Save original function pointer for EndScene
  vTable[42] = (DWORD)&hook;    //Replace VMT function pointer with the one to the function "hook" in this code
  VirtualProtect(&vTable[42], 4, NewProtection, &NewProtection);

  //Infinite loop
  while (1)
  {
    Sleep(1000);
  }
}

//Entry point
BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpvReserved)
{
  if (dwReason == DLL_PROCESS_ATTACH)
  {
    DisableThreadLibraryCalls(hModule);
    CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)MyHook, NULL, NULL, NULL);
  }
  return TRUE;
}