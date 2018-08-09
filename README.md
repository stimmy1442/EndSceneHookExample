# EndSceneHookExample

This is an example for hooking the Direct3D EndScene() function. It can be used for creating screen overlays like FPS counters in games.
Currently, only 32 bit DirectX 9 games are supported. To be honest, I just tested it with Halo and a Direct3D test application.

# Prerequisites
* Visual Studio 2017 (I used the free Community Edition)
* The Mircosoft DirectX SDK, can be downlaoded from https://www.microsoft.com/en-us/download/details.aspx?id=6812

# How to use
* Open the project "InjectedDLL". If necessary, change the VC++ directories so that the DirectX SDK is within the include and library paths
* Set the project configuration to 'Release' and 'x86', and build the project
* Open the project DLLInjector/DLLInjectV2 in Visual Studio
* Change the variables "HaloFile" and "HaloFolder" to the path/executable of the game you want to display the overlay in
* Copy the DLL from InjectedDLL\Release\InjectedDLL.dll to DLLInjector\Release or DLLInjector\Debug (depending on if you complie it on debug or release config)
* Set the project configuration to 'Release' and 'x86', compile the DLL injector and run it as administrator (the DLL injection needs admin rights)
* The DLL injector should now start the game, inject the DLL, and you should see the game with the screen overlay

# How does it work?
The DLL injection is pretty straightforward and explained in many places on the web. 
About EndScene hooking, I didn't easily find good tutorials and examples. That's why I decided to make + release a complete example.
I got most of the hooking code from https://www.unknowncheats.me/wiki/Direct3D:DirectX_9_EndScene_Midfunction_Hook_Example.
The hooking works in several steps:
One shoud know that 'EndScene()' is a member function of the 'IDirect3DDevice9' class. Therefore, it can't be hooked like a regular API call.
The function is called via the Virtual Method Table (VMT) of 'IDirect3DDevice9'.
When the DLL is injected into the game, its DLLMain is called. The DLLMain searches for the VMT in the memory via a magic number (this part of the code is not mine).
After finding the VMT, it backs up the function pointer to EndScene() in a variable ('dwEndscene_ret') before overwriting it with a pointer to a custom function.
This custom function now draws the overlay elements on the screen, and then calls the original EndScene().

For more information, refer to the included PowerPoint presentation, or to the explanatory YouTube video:
https://youtu.be/w3xn0zI0Fq0

# License
As said, some of the code is from https://www.unknowncheats.me/wiki/Direct3D:DirectX_9_EndScene_Midfunction_Hook_Example, no license is stated here.
Otherwise, feel free to use the code for whatever you like.