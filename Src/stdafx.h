#pragma once

#define WIN32_LEAN_AND_MEAN
#define USE_PIX

#ifdef _DEBUG
#define DX_VERIFY(x) {HRESULT hr = x; assert(hr == S_OK);}
#else
	#define DX_VERIFY(x) x
#endif

#pragma warning(disable : 4324)

#include <windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <d3dcompiler.h>

#include <array>
#include <cassert>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>
#include <DDSTextureLoader.h>
#include <DirectXMath.h>
#include <pix3.h>
#include <ResourceUploadBatch.h>