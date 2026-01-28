#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <algorithm>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <SimpleMath.h>
#include <wrl.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

#include "Helpers.h"

#include "WICTextureLoader.h"