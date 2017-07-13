#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <clocale>
#include <ctime>

#include <sstream>
#include <string>
#include <list>
#include <vector>
#include <deque>
#include <map>
#include <unordered_map>
#include <stack>
#include <cstdio>
#include <algorithm>
#include <bitset>
#include <stdint.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>
#include <dirent.h>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include <memory>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
//#include <windowsx.h>
#include <wrl/client.h>

#include <Shellapi.h>	
#include <Shlobj.h>	
#include <OleIdl.h>

#include <Dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")

#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

//#include <d3d11_1.h>
//#pragma comment(lib, "d3d11_1.lib")

#include <DirectXmath.h>
using namespace DirectX;
#include <SimpleMath.h>
using namespace SimpleMath;

#pragma comment(lib,"dxguid.lib")

#include <d3dcompiler.h>
#include <d3dcompiler.inl>
#pragma comment(lib,"d3dcompiler.lib")

#include <DDSTextureLoader.h>
#include <SimpleMath.h>
#pragma comment(lib,"DirectXTK.lib")

#include <DirectXTex.h>
#pragma comment(lib,"DirectXTex.lib")

#include <fstream>
using namespace std;

#include <lua.hpp>
#include <Luabridge.h>
using namespace luabridge;
//#pragma comment(lib,"lua51.lib")

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#undef max
#undef min

#include <reactphysics3d.h>
