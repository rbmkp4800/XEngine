#include <Windows.h>

#include "XEngine.Render.Internal.Shaders.h"

#include "..\Intermediate\Shaders\ScreenQuadVS.cso.h"
#include "..\Intermediate\Shaders\ClearDefaultUAVxCS.cso.h"
#include "..\Intermediate\Shaders\DepthBufferDownscalePS.cso.h"

#include "..\Intermediate\Shaders\LightingPassPS.cso.h"

#include "..\Intermediate\Shaders\OCxBBoxVS.cso.h"
#include "..\Intermediate\Shaders\OCxBBoxPS.cso.h"
#include "..\Intermediate\Shaders\OCxICLUpdateCS.cso.h"

#include "..\Intermediate\Shaders\DebugPositionOnlyVS.cso.h"
#include "..\Intermediate\Shaders\DebugWhitePS.cso.h"

#include "..\Intermediate\Shaders\UIColorVS.cso.h"
#include "..\Intermediate\Shaders\UIColorPS.cso.h"
#include "..\Intermediate\Shaders\UIFontVS.cso.h"
#include "..\Intermediate\Shaders\UIFontPS.cso.h"

#include "..\Intermediate\Shaders\EffectPlainVS.cso.h"
#include "..\Intermediate\Shaders\EffectPlainPS.cso.h"
#include "..\Intermediate\Shaders\EffectPlainSkinnedVS.cso.h"
#include "..\Intermediate\Shaders\EffectPlainSkinnedPS.cso.h"
#include "..\Intermediate\Shaders\EffectTextureVS.cso.h"
#include "..\Intermediate\Shaders\EffectTexturePS.cso.h"

using namespace XEngine::Internal;

ShaderData Shaders::ScreenQuadVS = { ScreenQuadVSData, sizeof(ScreenQuadVSData) };
ShaderData Shaders::ClearDefaultUAVxCS = { ClearDefaultUAVxCSData, sizeof(ClearDefaultUAVxCSData) };
ShaderData Shaders::DepthBufferDownscalePS = { DepthBufferDownscalePSData, sizeof(DepthBufferDownscalePSData) };

ShaderData Shaders::LightingPassPS = { LightingPassPSData, sizeof(LightingPassPSData) };

ShaderData Shaders::OCxBBoxVS = { OCxBBoxVSData, sizeof(OCxBBoxVSData) };
ShaderData Shaders::OCxBBoxPS = { OCxBBoxPSData, sizeof(OCxBBoxPSData) };
ShaderData Shaders::OCxICLUpdateCS = { OCxICLUpdateCSData, sizeof(OCxICLUpdateCSData) };

ShaderData Shaders::DebugPositionOnlyVS = { DebugPositionOnlyVSData, sizeof(DebugPositionOnlyVSData) };
ShaderData Shaders::DebugWhitePS = { DebugWhitePSData, sizeof(DebugWhitePSData) };

ShaderData Shaders::UIColorVS = { UIColorVSData, sizeof(UIColorVSData) };
ShaderData Shaders::UIColorPS = { UIColorPSData, sizeof(UIColorPSData) };
ShaderData Shaders::UIFontVS = { UIFontVSData, sizeof(UIFontVSData) };
ShaderData Shaders::UIFontPS = { UIFontPSData, sizeof(UIFontPSData) };

ShaderData Shaders::EffectPlainVS = { EffectPlainVSData, sizeof(EffectPlainVSData) };
ShaderData Shaders::EffectPlainPS = { EffectPlainPSData, sizeof(EffectPlainPSData) };
ShaderData Shaders::EffectPlainSkinnedVS = { EffectPlainSkinnedVSData, sizeof(EffectPlainSkinnedVSData) };
ShaderData Shaders::EffectPlainSkinnedPS = { EffectPlainSkinnedPSData, sizeof(EffectPlainSkinnedPSData) };
ShaderData Shaders::EffectTextureVS = { EffectTextureVSData, sizeof(EffectTextureVSData) };
ShaderData Shaders::EffectTexturePS = { EffectTexturePSData, sizeof(EffectTexturePSData) };