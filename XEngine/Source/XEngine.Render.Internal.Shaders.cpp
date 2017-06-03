#include <Windows.h>

#include "XEngine.Render.Internal.Shaders.h"

#include "Shaders\Compiled\LightingPassVS.cso.h"
#include "Shaders\Compiled\LightingPassPS.cso.h"

#include "Shaders\Compiled\DebugPositionOnlyVS.cso.h"
#include "Shaders\Compiled\DebugWhitePS.cso.h"

#include "Shaders\Compiled\UIFontVS.cso.h"
#include "Shaders\Compiled\UIFontPS.cso.h"

#include "Shaders\Compiled\EffectPlainVS.cso.h"
#include "Shaders\Compiled\EffectPlainPS.cso.h"
#include "Shaders\Compiled\EffectPlainSkinnedVS.cso.h"
#include "Shaders\Compiled\EffectPlainSkinnedPS.cso.h"
#include "Shaders\Compiled\EffectTextureVS.cso.h"
#include "Shaders\Compiled\EffectTexturePS.cso.h"

using namespace XEngine::Internal;

ShaderData Shaders::LightingPassPS = { LightingPassPSData, sizeof(LightingPassPSData) };
ShaderData Shaders::LightingPassVS = { LightingPassVSData, sizeof(LightingPassVSData) };

ShaderData Shaders::DebugPositionOnlyVS = { DebugPositionOnlyVSData, sizeof(DebugPositionOnlyVSData) };
ShaderData Shaders::DebugWhitePS = { DebugWhitePSData, sizeof(DebugWhitePSData) };

ShaderData Shaders::UIFontVS = { UIFontVSData, sizeof(UIFontVSData) };
ShaderData Shaders::UIFontPS = { UIFontPSData, sizeof(UIFontPSData) };

ShaderData Shaders::EffectPlainVS = { EffectPlainVSData, sizeof(EffectPlainVSData) };
ShaderData Shaders::EffectPlainPS = { EffectPlainPSData, sizeof(EffectPlainPSData) };
ShaderData Shaders::EffectPlainSkinnedVS = { EffectPlainSkinnedVSData, sizeof(EffectPlainSkinnedVSData) };
ShaderData Shaders::EffectPlainSkinnedPS = { EffectPlainSkinnedPSData, sizeof(EffectPlainSkinnedPSData) };
ShaderData Shaders::EffectTextureVS = { EffectTextureVSData, sizeof(EffectTextureVSData) };
ShaderData Shaders::EffectTexturePS = { EffectTexturePSData, sizeof(EffectTexturePSData) };