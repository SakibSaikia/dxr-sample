#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <pix3.h>
#include <array>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cassert>
#include <fstream>
#include <string>

#include <DDSTextureLoader.h>
#include <ResourceUploadBatch.h>
#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>
